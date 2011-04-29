bot
===

An IRC (rfc2812) bot written in C.

Installing
----------
$ make


Configuration
-------------

edit bot.ini to configure server, port and nick.
<pre>
[bot]
commandprefix=!
verbose=1       ; -1=quiet ; 0=info ; 1=error ; 2=debug
host=irc.freenode.net
port=6667
realname=Do I know you?
nick=strcspn
autojoin=##test4001
</pre>


Requirements
------------
* peg/leg from http://piumarta.com/software/peg/


Hacking
-------

fill_buffer() - reads from the irc socket
process_messages() - chops up buffer into lines.
decode_message() - chops up a line into prefix, command and arguments.
dispatch_message() - looks up function by command
on_xxx() - callbacks for IRC commands
on_privmsg() - if commandprefix found, then dispatch_user_command()
dispatch_user_command() - callbacks for bot commands (ex: !xxx)
on_user_xxx() - callbacks for bot commands


See Also
--------

See RFC2812 and RFC1459.

