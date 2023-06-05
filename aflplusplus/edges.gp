set terminal pdf color
set output "plot.pdf"
set datafile separator comma
set key noautotitle

plot [0:] [30000:] \
  "/home/regehr/tmp/out0/default/plot_data" using 1:13 with lines, \
  "/home/regehr/tmp/out1/default/plot_data" using 1:13 with lines, \
  "/home/regehr/tmp/out2/default/plot_data" using 1:13 with lines, \
  "/home/regehr/tmp/out3/default/plot_data" using 1:13 with lines, \
  "/home/regehr/tmp/out4/default/plot_data" using 1:13 with lines, \
  "/home/regehr/tmp/out5/default/plot_data" using 1:13 with lines, \
  "/home/regehr/tmp/out6/default/plot_data" using 1:13 with lines, \
  "/home/regehr/tmp/out7/default/plot_data" using 1:13 with lines, \
  "/home/regehr/tmp/out8/default/plot_data" using 1:13 with lines, \
  "/home/regehr/tmp/out9/default/plot_data" using 1:13 with lines, \
  "/home/regehr/tmp/out10/default/plot_data" using 1:13 with lines, \
  "/home/regehr/tmp/out11/default/plot_data" using 1:13 with lines, \
  "/home/regehr/tmp/out12/default/plot_data" using 1:13 with lines, \
  "/home/regehr/tmp/out13/default/plot_data" using 1:13 with lines, \
  "/home/regehr/tmp/out14/default/plot_data" using 1:13 with lines, \
  "/home/regehr/tmp/out15/default/plot_data" using 1:13 with lines
  
  