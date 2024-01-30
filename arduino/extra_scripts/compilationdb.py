# このファイルは，Compilation Databaseの生成にのみに使用されています．
# ビルドとは直接関係なく，LSPやIDE等のためのスクリプトです．
# see https://docs.platformio.org/en/latest/integration/compile_commands.html

Import("env")

env.Replace(COMPILATIONDB_INCLUDE_TOOLCHAIN=True)
