from pathlib import Path
import re
from opencc import OpenCC

ROOT = Path(r"D:\MuMain\src\Localization")

FILES = [
    ROOT / "Game.zh-CN.resx",
    ROOT / "Editor.zh-CN.resx",
    ROOT / "Metadata.zh-CN.resx",
]

cc = OpenCC("t2s")

value_pattern = re.compile(
    r"(<value>)(.*?)(</value>)",
    flags=re.DOTALL
)

for path in FILES:
    if not path.exists():
        print(f"[SKIP] 不存在: {path}")
        continue

    text = path.read_text(encoding="utf-8")

    backup = path.with_suffix(path.suffix + ".bak")
    if not backup.exists():
        backup.write_text(text, encoding="utf-8")

    def convert_value(match):
        before, value, after = match.groups()
        converted = cc.convert(value)
        return before + converted + after

    converted_text = value_pattern.sub(convert_value, text)

    path.write_text(converted_text, encoding="utf-8")

    print(f"[OK] {path.name}")