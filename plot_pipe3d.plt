set terminal wxt
set grid
set view equal xyz
while (1) {
splot \
"pipe3d.dat" using 1:2:3 with lines lw 2 title 'Pipe', \
"pipe3d.dat" every 5 using 1:2:3:(5*$4):(5*$5):(5*$6) with vectors head filled title 'Direction'
pause 1
}
