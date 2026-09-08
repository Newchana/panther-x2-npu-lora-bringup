import re, sys

SRC = "/tmp/oc/panther.dts"
OUT = "/tmp/oc/panther_m0cs0.dts"

with open(SRC, "r", encoding="utf-8", errors="replace") as f:
    txt = f.read()

start_marker = "\tspi@fe640000 {"
i = txt.find(start_marker)
if i < 0:
    print("spi node not found"); sys.exit(1)

# find matching closing brace of the node (brace depth)
depth = 0
j = i
while j < len(txt):
    c = txt[j]
    if c == '{':
        depth += 1
    elif c == '}':
        depth -= 1
        if depth == 0:
            break
    j += 1
node = txt[i:j+1]

# change pinctrl-0 (default) from m1 (0xda cs0 + 0xdb pins) to m0 cs0(0xdc)+m0 pins(0x2a3)
node2 = re.sub(r"pinctrl-0 = <0xda 0xdb>;", "pinctrl-0 = <0xdc 0x2a3>;", node)
# also neuter high-speed pinctrl-1 if present to avoid references to other groups (leave; not used with pinctrl-names=default only)
# remove st7789v child block
pat = re.compile(r"\n\t+st7789v@0 \{.*?\n\t+};", re.S)
node2, n = pat.subn("", node2)
print("removed st7789 children:", n)

# add spidev child before closing of node (before last '};' which closes node)
close = node2.rfind("};")
child = '''
\t\tspidev@0 {
\t\t\tcompatible = "rockchip,spidev";
\t\t\treg = <0x00>;
\t\t\tspi-max-frequency = <2000000>;
\t\t};
'''
node2 = node2[:close] + child + node2[close:]

txt = txt[:i] + node2 + txt[j+1:]
with open(OUT, "w", encoding="utf-8") as f:
    f.write(txt)
print("written", OUT)
