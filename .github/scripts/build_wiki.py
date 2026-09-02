from __future__ import annotations

import argparse
import re
import shutil
from pathlib import Path


PAGE_LINK_RE = re.compile(r"(?<!\!)\[([^\]]+)\]\(([^)]+)\)")
IMAGE_LINK_RE = re.compile(r"!\[([^\]]*)\]\(([^)]+)\)")
FENCE_RE = re.compile(r"^(```|~~~)")
INTERNAL_MD_RE = re.compile(r"^(?:\./)?([^/#?]+)\.md(?:#.*)?$")
IMAGE_PATH_RE = re.compile(r"^(?:\./)?image(s)?/(.+)$")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build GitHub Wiki pages from Document."
    )
    parser.add_argument("source", type=Path, help="Source document directory")
    parser.add_argument("output", type=Path, help="Output wiki directory")
    parser.add_argument(
        "--readme-as-home",
        action="store_true",
        help="Write README.md as HOME.md in the output.",
    )
    return parser.parse_args()


def page_title_for(path: Path) -> str | None:
    with path.open("r", encoding="utf-8") as handle:
        for line in handle:
            if line.startswith("# "):
                return line[2:].strip()
    return None


def convert_link_target(
    target: str,
    page_titles: dict[str, str],
    output_names: dict[str, str],
) -> str:
    if target.startswith(("http://", "https://", "#", "mailto:")):
        return target

    image_match = IMAGE_PATH_RE.match(target)
    if image_match:
        return f"images/{image_match.group(2)}"

    page_match = INTERNAL_MD_RE.match(target)
    if not page_match:
        return target

    page_file = f"{page_match.group(1)}.md"
    if page_file not in page_titles:
        return target

    anchor = ""
    if "#" in target:
        anchor = target[target.index("#") :]
    output_name = output_names.get(page_file, page_file)
    return Path(output_name).stem + anchor


def convert_page_link(
    match: re.Match[str],
    page_titles: dict[str, str],
    output_names: dict[str, str],
) -> str:
    target = match.group(2)
    converted_target = convert_link_target(target, page_titles, output_names)

    page_match = INTERNAL_MD_RE.match(target)
    if page_match:
        page_file = f"{page_match.group(1)}.md"
        title = page_titles.get(page_file)
        if title is not None:
            return f"[{title}]({converted_target})"

    return f"[{match.group(1)}]({converted_target})"


def convert_markdown(
    text: str,
    page_titles: dict[str, str],
    output_names: dict[str, str],
) -> str:
    lines = text.splitlines(keepends=True)
    output: list[str] = []
    in_fence = False

    for line in lines:
        if FENCE_RE.match(line):
            in_fence = not in_fence
            output.append(line)
            continue

        if in_fence:
            output.append(line)
            continue

        line = IMAGE_LINK_RE.sub(
            lambda match: f"![{match.group(1)}]({convert_link_target(match.group(2), page_titles, output_names)})",
            line,
        )
        line = PAGE_LINK_RE.sub(
            lambda match: convert_page_link(match, page_titles, output_names),
            line,
        )
        output.append(line)

    return "".join(output)


def copy_images(source: Path, output: Path) -> None:
    source_images = source / "images"
    if source_images.exists():
        shutil.copytree(source_images, output / "images", dirs_exist_ok=True)


def main() -> int:
    args = parse_args()
    source = args.source
    output = args.output

    markdown_files = sorted(source.glob("*.md"))
    if not markdown_files:
        raise FileNotFoundError(f"No markdown files found in {source}")

    page_titles = {path.name: page_title_for(path) for path in markdown_files}
    output_names = {path.name: path.name for path in markdown_files}
    if args.readme_as_home and "README.md" in output_names:
        output_names["README.md"] = "HOME.md"

    output.mkdir(parents=True, exist_ok=True)
    copy_images(source, output)

    for path in markdown_files:
        text = path.read_text(encoding="utf-8")
        converted = convert_markdown(text, page_titles, output_names)
        destination = output / output_names[path.name]
        destination.write_text(converted, encoding="utf-8", newline="\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
