#!/usr/bin/env python3
"""ビルド済み firmware.elf を addr2line 用に firmware_backup/ へ退避する。

ファイル名は elf の SHA256 上位16桁にする。これはデバイス起動時/クラッシュ時に
ログへ出る `ELF file SHA256:` の値と一致するため、後からクラッシュログのハッシュで
対応する elf を特定できる。

使い方:
    # デフォルト環境 (m5stack-cores3-release) の elf を退避
    python tools/save_elf.py

    # 環境名を指定して退避 (.pio/build/<env>/firmware.elf)
    python tools/save_elf.py -e m5stack-cores3

    # elf パスを直接指定 (-e より優先)
    python tools/save_elf.py --elf path/to/firmware.elf
"""
import argparse
import hashlib
import shutil
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_ENV = "m5stack-cores3-release"
BACKUP_DIR = REPO_ROOT / "firmware_backup"


def sha256_top16(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()[:16]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-e", "--environment", default=DEFAULT_ENV,
                        help=f"PlatformIO 環境名 (デフォルト: {DEFAULT_ENV})")
    parser.add_argument("--elf", type=Path, default=None,
                        help="elf パスを直接指定 (-e より優先)")
    args = parser.parse_args()

    elf = (args.elf.resolve() if args.elf
           else REPO_ROOT / ".pio" / "build" / args.environment / "firmware.elf")
    if not elf.is_file():
        print(f"[save_elf] elf が見つかりません: {elf}", file=sys.stderr)
        return 1

    digest = sha256_top16(elf)
    BACKUP_DIR.mkdir(exist_ok=True)
    dest = BACKUP_DIR / f"{digest}.elf"

    if dest.exists():
        print(f"[save_elf] 既に存在 (skip): {dest.name}")
        return 0

    shutil.copy2(elf, dest)
    print(f"[save_elf] 保存: firmware_backup/{dest.name}")
    print(f"[save_elf] クラッシュログの 'ELF file SHA256: {digest} ...' と対応します")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
