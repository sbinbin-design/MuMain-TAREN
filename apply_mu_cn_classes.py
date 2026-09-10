from pathlib import Path
import re

FILE = Path(r"D:\MuMain\src\Localization\Game.zh-CN.resx")

TERMS = {
    # 一转 / 二转 / 三转
    "Dark Wizard": "魔法师",
    "Soul Master": "魔导师",
    "Grand Master": "神导师",

    "Dark Knight": "剑士",
    "Blade Knight": "骑士",
    "Blade Master": "神骑士",

    "Elf": "弓箭手",
    "Muse Elf": "圣射手",
    "High Elf": "神射手",

    # 特殊职业
    "Magic Gladiator": "魔剑士",
    "Duel Master": "剑圣",

    "Dark Lord": "圣导师",
    "Lord Emperor": "祭师",
    # 召唤术师
    "Summoner": "召唤术师",
    "Bloody Summoner": "召唤导师",
    "Dimension Master": "召唤巫师",

    # 格斗家
    "Rage Fighter": "格斗家",
    "Fist Master": "格斗大师",
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