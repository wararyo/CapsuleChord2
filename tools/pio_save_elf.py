"""PlatformIO post-build フック: release ビルドの elf を firmware_backup/ へ退避する。

platformio.ini の release 環境で `extra_scripts = post:tools/pio_save_elf.py` として読み込む。
elf がビルド(再リンク)されるたびに tools/save_elf.py を呼び出し、
firmware_backup/[SHA256上位16桁].elf を生成する。
"""
import os

Import("env")  # noqa: F821  (PlatformIO が注入)


def _save_elf(source, target, env):
    elf = env.subst("$BUILD_DIR/${PROGNAME}.elf")
    script = os.path.join(env.subst("$PROJECT_DIR"), "tools", "save_elf.py")
    env.Execute(env.VerboseAction(
        f'"$PYTHONEXE" "{script}" --elf "{elf}"',
        "Saving elf to firmware_backup/",
    ))


env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", _save_elf)  # noqa: F821
