import argparse
from pathlib import Path
import time

import pandas as pd

from lib.models import VGG
from lib.models.qconfig import CustomQConfig
from lib.utils import load_model

from analytical_model import EyerissMapper
from network_parser import parse_pytorch, parse_onnx
from layer_info import Conv2DShapeParam, MaxPool2DShapeParam, ShapeParam
from roofline import plot_roofline_from_df


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        "model_path", type=str, help="path to the ONNX or PyTorch model"
    )
    parser.add_argument(
        "-f",
        "--format",
        type=str,
        default="torch",
        choices=["torch", "onnx"],
        help="input model format",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=str,
        default=f"../log/{time.strftime('%Y%m%d-%H%M%S')}",
        help="directory to save the output results",
    )
    parser.add_argument(
        "--plot",
        action="store_true",
        help="plot the roofline model and save it to the output directory",
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="print the results to stdout"
    )
    return parser.parse_args()


def parse_network(model_path: str, model_format: str) -> list[ShapeParam | None]:
    # Load the model
    model = load_model(
        VGG(),
        model_path,
        qconfig=CustomQConfig.POWER2.value,
        fuse_modules=True,
    )

    # Parse the network according to the model format
    _layers = []
    match model_format:
        case "torch":
            _layers = parse_pytorch(model)
        case "onnx":
            _layers = parse_onnx(model)
        case _:
            raise ValueError(f"Unsupported model format: {model_format}")

    # Convert the list of layers to a list of layers and None
    layers = []
    for i in range(len(_layers)):
        if isinstance(_layers[i], Conv2DShapeParam):
            layers.append(_layers[i])
            if not isinstance(_layers[i + 1], MaxPool2DShapeParam):
                layers.append(None)
        elif isinstance(_layers[i], MaxPool2DShapeParam):
            layers.append(_layers[i])
    return layers


def export_results(
    results: list[dict[str, str | int | float]], output_dir: str | Path
) -> None:
    # Create the output directory if it does not exist
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    # Save the results to a CSV file
    df = pd.DataFrame(results)
    df.to_csv(output_dir / "output.csv", index=False)

    # Save the results to a Markdown file
    markdown_table = df.to_markdown(index=False)
    with open(output_dir / "output.md", "w") as f:
        f.write("# Eyeriss Mapping Report\n\n")
        f.write("## Results\n\n")
        f.write(markdown_table)

    print(f"Report is saved to {output_dir}.")
    return df


def main():
    args = parse_args()
    model_path = Path(args.model_path).absolute()
    output_dir = Path(args.output).absolute()

    # Network parsing
    layers = parse_network(model_path, args.format)

    # Workload mapping and performance estimation
    results: list[dict[str, str | int | float]] = []
    for i in range(0, len(layers), 2):
        mapper = EyerissMapper(name=f"vgg8.conv{i // 2}")
        print(f"Running layer {i}: {layers[i]}")
        res = mapper.run(layers[i], layers[i + 1])
        results.extend(res)

    # Export the results to CSV and Markdown files
    df = export_results(results, output_dir)

    # Plot the roofline model
    if args.plot:
        plot_roofline_from_df(df, output_dir / "output.png")


if __name__ == "__main__":
    main()
