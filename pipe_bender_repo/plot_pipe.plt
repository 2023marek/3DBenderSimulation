cd "C:/Users/marek/source/repos/pipe_bender_repo/pipe_bender_repo"
set terminal wxt
set grid
set size ratio -1
plot \
"C:/Users/marek/source/repos/pipe_bender_repo/pipe_bender_repo/pipe.dat" using 1:2 with lines lw 2 title "Pipe", \
"C:/Users/marek/source/repos/pipe_bender_repo/pipe_bender_repo/pipe.dat" using 1:2:(75*cos($3)):(75*sin($3)) with vectors head filled lt 2 title "Direction"
pause -1
