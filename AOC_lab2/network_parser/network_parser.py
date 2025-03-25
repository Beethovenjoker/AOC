import sys
from pathlib import Path

import torch
import torch.nn as nn
import torch.nn.quantized as nnq
import onnx
from onnx import shape_inference
project_root = Path(__file__).parents[1]
sys.path.append(str(project_root))

from layer_info import (
    ShapeParam,
    Conv2DShapeParam,
    LinearShapeParam,
    MaxPool2DShapeParam,
)

from lib.models.vgg import VGG
import torch2onnx


def parse_pytorch(model: nn.Module, input_shape=(1, 3, 32, 32)) -> list[ShapeParam]:
    layers = []
    dummy_input = torch.randn(*input_shape)  # Test input
    hooks = []

    def hook_fn(module, inputs, output):
        input_shape = inputs[0].shape
        output_shape = output.shape

        if isinstance(module, (nn.Conv2d, nnq.Conv2d)):  # Parse convolution layer
            layers.append(
                Conv2DShapeParam(
                    N=input_shape[0],  # Batch size
                    H=input_shape[2], W=input_shape[3],  # Input image size
                    R=module.kernel_size[0], S=module.kernel_size[1],  # Kernel size
                    E=output_shape[2], F=output_shape[3],  # Output image size
                    C=input_shape[1], M=module.out_channels,  # Input/output channels
                    U=module.stride[0], P=module.padding[0]  # Stride & Padding
                )
            )
        elif isinstance(module, (nn.MaxPool2d, nnq.MaxPool2d)):  # Parse max pooling layer
            layers.append(
                MaxPool2DShapeParam(
                    N=input_shape[0], kernel_size=module.kernel_size, stride=module.stride
                )
            )
        elif isinstance(module, (nn.Linear, nnq.Linear)):  # Parse fully connected (linear) layer
            layers.append(
                LinearShapeParam(
                    N=input_shape[0], in_features=module.in_features, out_features=module.out_features
                )
            )

    # Set hooks to capture layer information
    for layer in model.modules():
        if isinstance(layer, (nn.Conv2d, nn.Linear, nn.MaxPool2d, nnq.Conv2d, nnq.Linear)):
            hooks.append(layer.register_forward_hook(hook_fn))

    # Perform a forward pass to trigger hooks
    model(dummy_input)

    # Remove hooks after capturing layer information
    for h in hooks:
        h.remove()

    return layers


def parse_onnx(model: onnx.ModelProto) -> list[ShapeParam]:
    layers = []

    # Use shape_inference to ensure ONNX contains all tensor shape information
    inferred_model = shape_inference.infer_shapes(model)
    graph = inferred_model.graph

    # Store all tensor shapes {tensor_name: shape}
    tensor_shapes = {}

    def get_shape(value_info):
        return [dim.dim_value for dim in value_info.type.tensor_type.shape.dim]

    # Extract tensor shape information from graph.input, graph.value_info, and graph.output
    for val in list(graph.input) + list(graph.value_info) + list(graph.output):
        tensor_shapes[val.name] = get_shape(val)

    # Iterate through all layers (nodes) in the ONNX model
    for node in graph.node:
        op_type = node.op_type  # e.g., 'Conv', 'MaxPool', 'QLinearConv', 'Gemm', 'QGemm'
        inputs = node.input
        outputs = node.output

        # Ensure valid input/output tensors exist
        if not inputs or not outputs or inputs[0] not in tensor_shapes or outputs[0] not in tensor_shapes:
            continue

        input_shape = tensor_shapes[inputs[0]]
        output_shape = tensor_shapes[outputs[0]]

        # Retrieve layer attributes (kernel_size, stride, padding, etc.)
        attrs = {attr.name: list(attr.ints) for attr in node.attribute}

        # Parse Conv2D or QLinearConv layer
        if op_type in ["Conv", "QLinearConv"]:
            kernel_size = attrs.get("kernel_shape", [3, 3])  # Default 3x3
            stride = attrs.get("strides", [1, 1])  # Default stride 1x1
            padding = attrs.get("pads", [1, 1, 1, 1])  # Default padding 1,1,1,1

            layers.append(
                Conv2DShapeParam(
                    N=input_shape[0], H=input_shape[2], W=input_shape[3],  # Input size
                    R=kernel_size[0], S=kernel_size[1],  # Kernel size
                    E=output_shape[2], F=output_shape[3],  # Output size
                    C=input_shape[1], M=output_shape[1],  # Input/output channels
                    U=stride[0], P=padding[0]  # Stride & Padding
                )
            )

        # Parse MaxPool2D layer
        elif op_type == "MaxPool":
            kernel_size = attrs.get("kernel_shape", [2, 2])[0]  # Default 2x2
            stride = attrs.get("strides", [2, 2])[0]  # Default stride 2
            layers.append(
                MaxPool2DShapeParam(
                    N=input_shape[0], kernel_size=kernel_size, stride=stride
                )
            )

        # Parse Fully Connected (Gemm or QGemm) layer
        elif op_type in ["Gemm", "QGemm"]:
            layers.append(
                LinearShapeParam(
                    N=input_shape[0], in_features=input_shape[1], out_features=output_shape[1]
                )
            )

    return layers

def compare_layers(answer, layers):
    if len(answer) != len(layers):
        print(
            f"Layer count mismatch: answer has {len(answer)}, but ONNX has {len(layers)}"
        )

    min_len = min(len(answer), len(layers))

    for i in range(min_len):
        ans_layer = vars(answer[i])
        layer = vars(layers[i])

        diffs = {
            k: (ans_layer[k], layer[k])
            for k in ans_layer
            if k in layer and ans_layer[k] != layer[k]
        }

        if diffs:
            print(f"Difference in layer {i + 1} ({type(answer[i]).__name__}):")
            for k, (ans_val, val) in diffs.items():
                print(f"  {k}: answer = {ans_val}, onnx = {val}")

    if len(answer) > len(layers):
        print(f"Extra layers in answer: {answer[len(layers) :]}")
    elif len(layers) > len(answer):
        print(f"Extra layers in yours: {layers[len(answer) :]}")


def run_tests() -> None:
    """Run tests on the network parser functions for the new model."""
    answer = [
        Conv2DShapeParam(N=1, H=32, W=32, R=3, S=3, E=32, F=32, C=3, M=32, U=1, P=1),
        MaxPool2DShapeParam(N=1, kernel_size=2, stride=2),
        Conv2DShapeParam(N=1, H=16, W=16, R=3, S=3, E=16, F=16, C=32, M=64, U=1, P=1),
        MaxPool2DShapeParam(N=1, kernel_size=2, stride=2),  # 16x16 → 8x8
        Conv2DShapeParam(N=1, H=8, W=8, R=3, S=3, E=8, F=8, C=64, M=128, U=1, P=1),
        Conv2DShapeParam(N=1, H=8, W=8, R=3, S=3, E=8, F=8, C=128, M=128, U=1, P=1),
        Conv2DShapeParam(N=1, H=8, W=8, R=3, S=3, E=8, F=8, C=128, M=128, U=1, P=1),
        MaxPool2DShapeParam(N=1, kernel_size=2, stride=2),  # 8x8 → 4x4
        LinearShapeParam(N=1, in_features=2048, out_features=128),  # 128 * 4 * 4 = 2048
        LinearShapeParam(N=1, in_features=128, out_features=64),
        LinearShapeParam(N=1, in_features=64, out_features=10),
    ]


    # Test with the PyTorch model.
    model = VGG()
    layers_pth = parse_pytorch(model)

    # Define the input shape.
    dummy_input = torch.randn(1, 3, 32, 32)
    # Save the model to ONNX.
    torch2onnx.torch2onnx(model, "parser_onnx.onnx", dummy_input)
    # Load the ONNX model.
    model_onnx = onnx.load("parser_onnx.onnx")
    layers_onnx = parse_onnx(model_onnx)

    # Display results.
    print("PyTorch Network Parser:")
    if layers_pth == answer:
        print("Correct!")
    else:
        print("Wrong!")
        compare_layers(answer, layers_pth)

    print("ONNX Network Parser:")
    if layers_onnx == answer:
        print("Correct!")
    else:
        print("Wrong!")
        compare_layers(answer, layers_onnx)


if __name__ == "__main__":
    run_tests()
