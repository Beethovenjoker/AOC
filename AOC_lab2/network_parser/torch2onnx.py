import torch


def torch2onnx(model, output_file_path, dummy_input):
    torch.onnx.export(
        model,  # Model to export
        dummy_input,  # Dummy input for the model
        output_file_path,  # Output file path
        export_params=True,  # Export the model parameters (weights)
        opset_version=11,  # ONNX opset version
        do_constant_folding=True,  # Optimize constants (e.g., remove redundant operations)
        input_names=["input"],  # Name of input layer
        output_names=["output"],  # Name of output layer
        dynamic_axes=None,
    )

    print(f"Model saved as {output_file_path}")
