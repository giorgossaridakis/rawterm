# rawterm
UNIX telnet terminal with raw input-output

Record ASCII below 32, keep logs, show ASCII codes

telnet in complete raw output mode
--------------------------------------------------
Usage:
 rawterm [options] <server> <port> 

An unobstructed telnet terminal.

Options:
 -d		ASCII decimal codes OFF
 -c		ASCII key codes OFF
 -s		display non-screen chars ON
 -a		interpet ANSI ON
 -l		log file OFF
 -o<filename>	output file, default <rawterm.log>
      --help	display this help

Distributed under the GNU Public licence.
