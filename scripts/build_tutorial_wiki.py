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
LANGUAGE_SUFFIX_RE = re.compile(r"\.(en|ja)\.md$", re.IGNORECASE)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build GitHub Wiki pages from Document/tutorial."
    )
    parser.add_argument("source", type=Path, help="Source tutorial directory")
    parser.add_argument("output", type=Path, help="Output wiki directory")
    return parser.parse_args()


def sanitize_page_name(name: str) -> str:
    sanitized = name.replace("/", "-").replace("\\", "-").replace(":", " -")
    sanitized = re.sub(r"\s+", " ", sanitized).strip()
    if not sanitized:
        raise ValueError("Page name is empty after sanitization")
    return sanitized


def detect_language(path: Path) -> str | None:
    match = LANGUAGE_SUFFIX_RE.search(path.name)
    if not match:
        return None
    return match.group(1).lower()


def page_name_for(path: Path) -> str:
    if path.name in {"Home.md", "_Sidebar.md", "_Footer.md"}:
        return path.stem

    with path.open("r", encoding="utf-8") as handle:
        for line in handle:
            if line.startswith("# "):
                title = line[2:].strip()
                break
        else:
            raise ValueError(f"Missing level-1 heading in {path}")

    language = detect_language(path)
    if language is None:
        return sanitize_page_name(title)

    suffix = language.upper()
    return sanitize_page_name(f"{title} ({suffix})")


def convert_link_target(target: str, page_names: dict[str, str]) -> str:
    if target.startswith(("http://", "https://", "#", "mailto:")):
        return target

    image_match = IMAGE_PATH_RE.match(target)
    if image_match:
        return f"images/{image_match.group(2)}"

    page_match = INTERNAL_MD_RE.match(target)
    if not page_match:
        return target

    page_file = f"{page_match.group(1)}.md"
    if page_file not in page_names:
        return target

    anchor = ""
    if "#" in target:
        anchor = target[target.index("#") :]
    return page_names[page_file] + anchor


def convert_markdown(text: str, page_names: dict[str, str]) -> str:
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
            lambda match: f"![{match.group(1)}]({convert_link_target(match.group(2), page_names)})",
            line,
        )
        line = PAGE_LINK_RE.sub(
            lambda match: f"[{match.group(1)}]({convert_link_target(match.group(2), page_names)})",
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

    page_names = {path.name: page_name_for(path) for path in markdown_files}
    duplicate_names = {
        name for name in page_names.values() if list(page_names.values()).count(name) > 1
    }
    if duplicate_names:
        duplicates = ", ".join(sorted(duplicate_names))
        raise ValueError(f"Duplicate wiki page names detected: {duplicates}")

    output.mkdir(parents=True, exist_ok=True)
    copy_images(source, output)

    for path in markdown_files:
        text = path.read_text(encoding="utf-8")
        converted = convert_markdown(text, page_names)
        destination = output / f"{page_names[path.name]}.md"
        destination.write_text(converted, encoding="utf-8", newline="\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
