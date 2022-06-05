#!/usr/bin/env python
# -*- coding: UTF-8 -*-
# Copyright: Song Huang <song@song.idv.tw>
# License: GUN GPL
# 2004/10/26

import popen2
from optparse import OptionParser
from codecs import open as cOpen
from string import letters, punctuation

def parsePO(po):
    try:
        poFile = cOpen(po, 'r', 'utf8')
        print '>> parse ', po
        for line in poFile.readlines():
            if line[:6] == 'msgstr':
                line = line.strip()
                for s in line[8:-1]:
                    s = str(ord(s))
                    if s not in stringList:
                        stringList.append(s)
        poFile.close()
    except IOError, e:
        print "Unable to open the file:" , po, e

if __name__ == '__main__':
    # parse script arguments
    optparser = OptionParser(usage='./%prog [options] original_font_file', version="%prog 0.2")
    optparser.add_option("-l", "--locale", action="store",  help="to make the locale fonts subset")
    optparser.add_option("-p", "--pofile", action="append", help="parse the pofile to get strings")
    (options, args) = optparser.parse_args()

    # get all words
    if options.locale and options.pofile and args:
        stringList = []
        for c in list(letters + punctuation):
            stringList.append(str(ord(c)))
        for po in options.pofile:
            parsePO(po)
        stringList.sort()
        #print "poList = ", options.pofile, "\nstringList = ", stringList
    else:
        print "Error: lost some option or original font file, please run the script with --help argument."

    # make font subset
    cmd = "./tuxpaintsubset.pe %s %s.ttf %s" % (args[0], options.locale, ' '.join(stringList))
    print cmd
    r, w, e = popen2.popen3(cmd)
    msg = r.read()
    if msg:
        print msg
    error = e.read()
    if error:
        print error
    r.close()
    w.close()
    e.close()
