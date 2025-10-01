import subprocess

Import("env")

def gitrev_build_flag():
    ret = subprocess.run(
        ["git", "describe", "--always", "--dirty"],
        capture_output=True,
        check=True,
    )
    gitrev = ret.stdout.strip().decode("utf8").upper()
    build_flag = f'-DGIT_REVISION=\\"g{gitrev}\\"'
    print(f"Defining git revision via build flag: {build_flag!r}")
    return build_flag

env.Append(BUILD_FLAGS=[gitrev_build_flag()])
