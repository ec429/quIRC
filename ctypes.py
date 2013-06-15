#!/usr/bin/python

logtypes = ('logtype', 'LOGT_', ['PLAIN', 'SYMBOLIC', 'NONE'])
btypes = ('btype', 'BT_', ['STATUS', 'SERVER', 'CHANNEL', 'PRIVATE'])
mtypes = ('mtype', 'MT_', ['MSG', 'NOTICE', 'PREFORMAT', 'ACT', 'JOIN', 'PART', 'QUIT', 'QUIT_PREFORMAT', 'NICK', 'MODE', 'STATUS', 'ERR', 'UNK', 'UNK_NOTICE', 'UNN'])
prios = ('prio', 'PRIO_', [('QUIET', 'hide in quiet mode'), ('NORMAL', 'always show'), ('DEBUG', 'show only in debug mode')])
rxmodes = ('rxmode', 'RXM_', ['nil', '0', 'sym'])

types = (logtypes, btypes, mtypes, prios, rxmodes)
