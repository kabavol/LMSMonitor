import math
# same polar arc math as used in RGB info panel, pythonized


def polarToCartesian(centerX, centerY, radius, angleInDegrees):
    theta = (angleInDegrees-90) * math.pi/180.0

    return {
        'x': centerX + (radius * math.cos(theta)),
        'y': centerY + (radius * math.sin(theta))
    }


def describeArc(x, y, radius, startAngle, endAngle):

    start = polarToCartesian(x, y, radius, endAngle)
    end = polarToCartesian(x, y, radius, startAngle)

    largeArcFlag = ("0" if (endAngle - startAngle <= 180) else "1")

    d = [
        "M", str(start['x']), str(start['y']),
        "A", str(radius), str(radius), '0', largeArcFlag, '0', str(
            end['x']), str(end['y'])
    ]

    return ' '.join(d)


print(f'''<svg width="19" height="{18*19}"
 xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
<defs>
    <mask id="hole">
        <rect width="19" height="19" fill="white"/>
        <circle r="9" cx="9.5" cy="9.5" fill="black"/>
        <rect width="19" height="5" y="2.5" fill="white"/>
        <rect width="19" height="2" y="8.5" fill="white"/>
    </mask>
    <symbol width="19" height="19" id="label">
        <circle id="lcore" fill="white" r="9.5" cx="9.5" cy="9.5" mask="url(#hole)" />
    </symbol>
    <pattern id="checks" x="0" y="0" width=".1" height=".11111111111111111111111111">
        <rect x="0" y="0" width="1" height="1" fill="black"/>
        <rect x="1" y="1" width="1" height="1" fill="black"/>
    </pattern>
</defs>
<rect width="110%" height="110%" x="-1" y="-1" fill="black"/>
''')
for y in range(0, 18):
    a = 20 * y
    z = (19*(1+y))-9.5
    c = 'black'
    print(
        f'<use xlink:href="#label" x="0" y="{19*y}" width="19" transform="rotate({a} 9.5 {z})"/>'
        f'<circle fill="url(#checks)" r="9" cx="9.5" cy="{z}"/>'
        f'<circle fill="{c}" r="1.5" cx="9.5" cy="{z}"/>'
        f'<path fill="none" stroke-linecap="round" stroke="{c}" stroke-width="2" d="{describeArc(9.5, z, 4.5, 8, 178)}"/>'
    )
print('</svg>')

print(f'''<svg width="40" height="{12*54}"
 xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">

<defs>
<symbol width="7" height="11" id="arm">
  <path d="m-0.03125,1.11719l7.71875,0.00781l-0.125,5.25l-2.8125,-0.0625l-0.125,10.125l5.75,15.1875l-2.0625,4.875l1,0.6875l-2.25,5.875l4,1.5l-0.5625,0.875l-3.75,-1.1875l-2,4.9375l-4.4375,-1.5625l5,-11.6875l0.9375,0.125l1.8125,-3.875l-5.5625,-14.9375l0,-10.6875l-2.5,-0.375l-0.03125,-5.07031z"/>
  <rect height="1" width="8" y="3" x="0" fill="black"/>
  <circle fill="black" cy="46.24964" cx="2.31258" r="0.8"/>
  <circle fill="black" cy="44.37470" cx="3.25005" r="0.8"/>
  <circle fill="black" cy="42.62475" cx="4.06253" r="0.8"/>
  <circle fill="black" cy="40.74981" cx="4.75001" r="0.8"/>
  <circle fill="black" cy="47.24964" cx="4.12508" r="0.8"/>
  <circle fill="black" cy="45.24970" cx="4.87505" r="0.8"/>
  <circle fill="black" cy="43.31225" cx="5.68753" r="0.8"/>
  <circle fill="black" cy="41.37481" cx="6.31251" r="0.8"/>
</symbol>
</defs>
<rect width="110%" height="110%" x="-1" y="-1" fill="black"/>
''')
z = 0
yy = 0
x = 23
for y in range(0, 12):
    if y > 0:
        z = (2*y) + 23
    print(f'<use xlink:href="#arm" x="{x}" y="{3+(54*yy)}" ',
          f'transform="rotate({z} {x+4} {14+(54*yy)})" fill="white"/>')
    yy += 1
print('</svg>')

cnt = 18
siz = 30
print(f'''<svg width="{siz}" height="{siz*cnt}"
 xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
<def>
    <symbol width="{(siz/2)-1}" height="{(siz/2)-1}" id="ko">
        <circle fill="black" r="{((siz/2)-5)/2}"/>
    </symbol>

    <symbol width="{siz}" height="{siz}" id="reel">
  <circle fill="white" cy="{siz/2}" cx="{siz/2}" r="{(siz/2)-1}"/>''')

for y in range(0, 5):
    a = 72 * y
    print(
        f'<use xlink:href="#ko" x="7.5" y="8.5" transform="rotate({a} {siz/2} {siz/2})"/>')

print(f'<circle fill="black" r="4.5" cx="{siz/2}" cy="{siz/2}"/>')

for y in range(0, 6):
    a = 60 * y
    print(
        f'<circle fill="white" r="1" cx="{(siz/2)-3}" cy="{siz/2}" transform="rotate({a} {siz/2} {siz/2})"/>')

print(f'''
<line fill="none" stroke-linecap="round" stroke="black" stroke-width="2" x1="0" y1="{siz/2}" x2="6" y2="{siz/2}"/>
    </symbol>
</def>
  <rect width="110%" height="110%" x="-1" y="-1" fill="black"/>
''')

for y in range(0, cnt):
    a = 20 * y
    z = (siz*(1+y))-(siz/2)
    print(
        f'<use xlink:href="#reel" x="0" y="{siz*y}" width="{siz}" transform="rotate({a} {siz/2} {z})"/>')

print('</svg>')

# vent layout for 90x150 emclosure lid
h = 90
w = 150
print(f'''<?xml version="1.0" standalone="no"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" 
  "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg width="{w}mm" height="{h}mm" version="1.1"
     viewBox="0 0 {w} {h}" xmlns="http://www.w3.org/2000/svg">''')
print(f'<g><rect fill="white" width="{w}" height="{h}" x="0" y="0"/>')
# 5V fan
screwspacing = 24
lpos = (w-screwspacing)/2.00
rpos = (w+screwspacing)/2.00
ypos = 70
r = 3.50/2.00

print(f'''<defs>
<symbol id="vent">
<line x1="2" y1="2" x2="2" y2="17" fill="none" stroke-linecap="round" stroke="red" stroke-width="2.8"/>
<line y1="27" x1="2" y2="42" x2="2" fill="none" stroke-linecap="round" stroke="red" stroke-width="2.8"/>
<line y1="52" x1="2" y2="67" x2="2" fill="none" stroke-linecap="round" stroke="red" stroke-width="2.8"/>
</symbol>
</defs>
<g id="fanKnockout">
   <circle fill="black" cy="{lpos}" cx="{ypos}" r="{r}"/>
   <circle fill="black" cy="{rpos}" cx="{ypos}" r="{r}"/>
   <circle fill="black" cy="{lpos}" cx="{ypos+screwspacing}" r="{r}"/>
   <circle fill="black" cy="{rpos}" cx="{ypos+screwspacing}" r="{r}"/>
''')

cxpos = w/2.00
cypos = ypos+(screwspacing/2.00)
c = "black"
sw = 3

iter = 8
turn = 360/iter
for y in range(0, iter):
    sw = 3
    r = 13
    degs = (turn * (iter/r)) / 2.00
    ha = degs + (turn*y)
    la = ha - (degs * 2)
    print(
        #f'    <path fill="none" stroke-linecap="round" stroke="{c}" stroke-width="{sw}" d="{describeArc(cxpos, cypos, r, la, ha)}"/>')
        f'    <path fill="none" stroke-linecap="round" stroke="{c}" stroke-width="{sw}" d="{describeArc(cypos, cxpos, r, la, ha)}"/>')
    r = 9
    #sw -= .5
    degs = 9.5 # (turn * (iter/((y+1)*r))) / 2.00
    ha = degs + (turn*y)
    la = ha - (degs * 2)
    print(
        #f'    <path fill="none" stroke-linecap="round" stroke="{c}" stroke-width="{sw}" d="{describeArc(cxpos, cypos, r, la, ha)}"/>')
        f'    <path fill="none" stroke-linecap="round" stroke="{c}" stroke-width="{sw}" d="{describeArc(cypos, cxpos, r, la, ha)}"/>')
    r = 5
    degs = 2 # (turn * (iter/((y+1)*r))) / 2.00
    ha = degs + (turn*y)
    la = ha - (degs * 2)
    print(
        #f'    <path fill="none" stroke-linecap="round" stroke="{c}" stroke-width="{sw}" d="{describeArc(cxpos, cypos, r, la, ha)}"/>')
        f'    <path fill="none" stroke-linecap="round" stroke="{c}" stroke-width="{sw}" d="{describeArc(cypos, cxpos, r, la, ha)}"/>')

print('\n</g>\n<g id="venting">')

l = 65
mw = w/2.00
xoff = 4
xpos = (mw-(l+xoff))

for y in range(0, 5):
    ypos = 15 + (y*10)
    print(
        f'<use xlink:href="#vent" y="{xpos}" x="{ypos}"/>'
    )
    if y < 4:
        print(
            f'<use xlink:href="#vent" y="{xpos}" x="{ypos+89}"/>'
        )

print('</g></g>')
print('</svg>')
