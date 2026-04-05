set terminal wxt
set grid
set size ratio -1
while (1) {
plot \
"pipe.dat" using 1:2 with lines lw 2 title 'Pipe', \
"pipe.dat" every 5 using 1:2:(5*cos($3)):(5*sin($3)) with vectors head filled title 'Direction'
pause 1
}
