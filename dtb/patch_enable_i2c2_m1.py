import re, sys

SRC = "/tmp/oc/panther.dts"
OUT = "/tmp/oc/panther_i2c2.dts"

with open(SRC, "r", encoding="utf-8", errors="replace") as f:
    txt = f.read()

marker = "\ti2c@fe5b0000 {"
i = txt.find(marker)
if i < 0:
    print("i2c2 node not found"); sys.exit(1)
depth = 0; j = i
while j < len(txt):
    c = txt[j]
    if c == '{': depth += 1
    elif c == '}':
        depth -= 1
        if depth == 0: break
    j += 1
node = txt[i:j+1]

node2 = node.replace('pinctrl-0 = <0xca>;', 'pinctrl-0 = <0x231>;')
node2 = node2.replace('status = "disabled";', 'status = "okay";', 1)
txt = txt[:i] + node2 + txt[j+1:]
with open(OUT, "w", encoding="utf-8") as f:
    f.write(txt)
print("patched i2c2 -> m1(0x231), status okay")
print("node snippet:")
for line in node2.splitlines()[:10]:
    print("  " + line)
