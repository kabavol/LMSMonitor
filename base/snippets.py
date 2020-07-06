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
        "A", str(radius), str(radius), '0', largeArcFlag, '0', str(end['x']), str(end['y'])
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
        f'<path fill="none" stroke="{c}" stroke-width="2" d="{describeArc(9.5, z, 4.5, 8, 178)}"/>'
    )
print('</svg>')
'''
+23
  32
14

+43deg max
  25
24
'''
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
