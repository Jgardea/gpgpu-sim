# Gnuplot 
#set terminal latex
#set output "test.tex"
set terminal pngcairo size 350,262 enhanced font 'Verdana,10'
set output 'test.png' 

#set     termoption enhanced 
set     title graph_title
set     style data linespoints

set     xlabel "Injection Rate(Packets/Node/Cycle)"
set     ylabel "Packet Latency (Cycles)"
set     autoscale
set     yrange [0.0025:150]

set     style line 1 lt rgb "cyan" lw 2 pt 7
set     style line 2 lt rgb "red"  lw 2 pt 8
set     style line 3 lt rgb "green" lw 2 pt 5
set     style line 4 lt rgb "orange" lw 2 pt 3

unset   log
set     xtic nomirror
set     ytic auto nomirror

plot 3DMesh            ls 1, \
     VB-3DMesh         ls 2, \
     4-Sym_VB-3DMesh   ls 3, \
     2-Asym_VB-3DMesh  ls 4
