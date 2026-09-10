from pathlib import Path
import re

FILE = Path(r"D:\MuMain\src\Localization\Game.zh-CN.resx")

# 第一批：经典大陆 MU 常用术语
TERMS = {
    # 职业
    "Dark Wizard": "魔法师",
    "Dark Knight": "剑士",
    "Elf": "弓箭手",
    "Magic Gladiator": "魔剑士",
    "Dark Lord": "圣导师",

    # 地图
    "Lorencia": "勇者大陆",
    "Dungeon": "地下城",
    "Devias": "冰风谷",
    "Noria": "仙踪林",
    "Lost Tower": "失落之塔",
    "Arena": "竞技场",
    "Atlans": "亚特兰蒂斯",
    "Tarkan": "死亡沙漠",
    "Icarus": "天空之城",

    # 活动
    "Devil Square": "恶魔广场",
    "Blood Castle": "血色城堡",
    "Chaos Castle": "混沌城堡",
}

text = FILE.read_text(encoding="utf-8")

changed = 0

for key, value in TERMS.items():
    pattern = re.compile(
        r'(<data\s+name="' + re.escape(key) +
        r'"\s+xml:space="preserve">\s*<value>)(.*?)(</value>)',
        re.DOTALL
    )

    text, count = pattern.subn(
        lambda m, v=value: m.group(1) + v + m.group(3),
        text
    )

    if count:
        print(f"[OK] {key} -> {value}")
        changed += count
    else:
        print(f"[MISS] {key}")

FILE.write_text(text, encoding="utf-8")

print()
print(f"完成，共修改 {changed} 项。")