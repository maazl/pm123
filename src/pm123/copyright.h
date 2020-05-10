/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp�  <rosmo@sektori.com>
 * Copyright 2004-2007 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2006-2008 Marcel Mueller <pm123@maazl.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *    3. The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PM123_COPYRIGHT_H
#define PM123_COPYRIGHT_H

#define AMP_VERSION  "1.42"
#define AMP_NAME     "PM123"
#define AMP_FULLNAME AMP_NAME " " AMP_VERSION

#ifndef RC_INVOKED
#define SDG_AUT "Marcel Mueller (http://www.maazl.de/)\n"\
                "Dmitry A.Steklenev <glass@ptv.ru> (http://glass.spb.ru/)\n"\
                "Samuel Audet <guardia@step.polymtl.ca> (http://step.polymtl.ca/~guardia/)\n"\
                "Taneli Lepp\204 <rosmo@sektori.com>\n"\
                "Michael Hipp, Oliver Fromme (original mpg123: http://www.mpg123.org)"

#define SDG_MSG "Rosmo's greetings go out to: Sektori.com folks, dink (check out his cool Z! MP3 player), Olli 'MrWizard' Maksimainen, Adrian 'ktk' Gschwend (Netlabs rules!), ml, Eirik 'Ltning' Overby, tSS, PartyGirl, Kerni, NuKe, WinAmp authors for inspiration, #os2finnin kanta-asiakkaat, #os2prog'ers (now #codepros no?), Sinebrychoff and all MP3 enthuastics.\n\n"\
                "Samuel's greetings: Mom, Miho, Sophie, Gerry, Marc, Olli Maksimainen, Mads Orbesen Troest (Leech it!), Eirik Overby (made some cool skins!), dink, nuke (EverBlue ready now?? :), Adrian Gschwend (aka ktk), GryGhost, Julien Pierre (who helped me with MMOS/2 before), Cow_woC, Sander van Leeuwen, Achim Hasenmueller (lou!! WHAT? Odin not ready yet? :) and of course Michael Hipp and Oliver Fromme!!\n\n"\
"\n"\
"Please read first the important information in the COPYRIGHT and COPYING "\
"files that have been copied below.\n"\
"\n"\
"Original mpg123 decoder written/modified by:\n"\
"   Michael Hipp <Michael.Hipp@student.uni-tuebingen.de>\n"\
"   Oliver Fromme <oliver.fromme@heim3.tu-clausthal.de>\n"\
"\n"\
"with help and code from:\n"\
"   Samuel Audet <guardia@step.polymtl.ca>: OS/2 port\n"\
"   MPEG Software Simulation Group: reference decoder package\n"\
"   Tobias Bading: idea for DCT64 in subband synthesis from maplay package\n"\
"   Jeff Tsay and Mikko Tommila: MDCT36 from maplay package\n"\
"   Philipp Knirsch <phil@mpik-tueb.mpg.de>: DCT36/manual unroll idea\n"\
"   Niklas Beisert <nbeisert@physik.tu-muenchen.de> MPEG 2.5 tables\n"\
"   Henrik P Johnson: HTTP auth patch\n"\
"\n"\
"Graphical User Interface for pm123 by:\n"\
"\n"\
"   Taneli Lepp� <rosmo@sektori.com>\n"\
"   Samuel Audet <guardia@step.polymtl.ca>\n"\
"   Dmitry A.Steklenev <glass@ptv.ru>\n"\
"   Marcel Mueller (http://www.maazl.de/)\n"\
"\n"\
"Generalized Bitmap Module by:\n"\
"\n"\
"   Andy Key <nyangau@interalpha.co.uk>\n"\
"   http://www.interalpha.net/customer/nyangau/\n"\
"\n"\
"Fast Fourier Transformation Library:\n"\
"\n"\
"   FFT123.DLL contains free collection of fast C routines for computing the "\
"   Discrete Fourier Transform in one or more dimensions. FFTW was written by "\
"   Matteo Frigo and Steven G. Johnson.  You can contact them at fftw@fftw.org. "\
"   The latest version of FFTW, benchmarks, links, and other information can "\
"   be found at the FFTW home page (http://www.fftw.org).\n"\
"\n"\
"COPYING\n"\
"=======\n"\
"\n"\
"This license covers ONLY libmpg123.dll and the related source files.\n"\
"\n"\
"Copyright (c) 1995-1999 by Michael Hipp, all rights reserved.\n"\
"Copyright (c) 1998-2003 by Samuel Audet, all rights reserved.\n"\
"Copyright (c) 2004-2007 by Dmitry A.Stekelnev, all rights reserved.\n"\
"\n"\
"Original mpg123 decoder written/modified by:\n"\
"   Michael Hipp <Michael.Hipp@student.uni-tuebingen.de>\n"\
"   Oliver Fromme <oliver.fromme@heim3.tu-clausthal.de>\n"\
"\n"\
"with help and code from:\n"\
"   Samuel Audet <guardia@step.polymtl.ca>: OS/2 port\n"\
"   MPEG Software Simulation Group: reference decoder package\n"\
"   Tobias Bading: idea for DCT64 in subband synthesis from maplay package\n"\
"   Jeff Tsay and Mikko Tommila: MDCT36 from maplay package\n"\
"   Philipp Knirsch <phil@mpik-tueb.mpg.de>: DCT36/manual unroll idea\n"\
"   Thomas Woerner <..> (SGI Audio)\n"\
"   Damien Clermonte <..> (HP-UX audio fixes)\n"\
"   Stefan Bieschewski <stb@acm.org>: Pentium optimizations, decode_i586.s\n"\
"   Martin Denn <mdenn@unix-ag.uni-kl.de>: NAS port\n"\
"   Niklas Beisert <nbeisert@physik.tu-muenchen.de> MPEG 2.5 tables\n"\
"   <mycroft@NetBSD.ORG> and <augustss@cs.chalmers.se>: NetBSD\n"\
"   Kevin Brintnall <kbrint@visi.com> BSD patch\n"\
"   Tony Million: win32 port\n"\
"   Steven Tiger Lang: advanced shuffle\n"\
"   Henrik P Johnson: HTTP auth patch\n"\
"   and more ....\n"\
"\n"\
"Current distribution site for new test versions is the CVS repository. "\
"http://www.mpg123.org/ has more information about how to acces the "\
"CVS repository.\n"\
"\n"\
"You can get the latest release from\n"\
"  http://www.mpg123.org/\n"\
"\n"\
"DISTRIBUTION:\n"\
"This software may be distributed freely, provided that it is "\
"distributed in its entirety, without modifications, and with "\
"the original copyright notice and license included. You may "\
"distribute your own seperate patches together with this software "\
"package or a modified package if you always include a pointer "\
"where to get the original unmodified package. In this case "\
"you must inform the author about the modified package. "\
"The software may not be sold for profit or as \"hidden\" part of "\
"another software, but it may be included with collections "\
"of other software, such as CD-ROM images of FTP servers and "\
"similar, provided that this software is not a significant part "\
"of that collection.\n"\
"Precompiled binaries of this software may be distributed in the "\
"same way, provided that this copyright notice and license is "\
"included without modification.\n"\
"\n"\
"USAGE:\n"\
"This software may be used freely, provided that the original "\
"author is always credited.  If you intend to use this software "\
"as a significant part of business (for-profit) activities, you "\
"have to contact the author first.  Also, any usage that is not "\
"covered by this license requires the explicit permission of "\
"the author.\n"\
"\n"\
"DISCLAIMER:\n"\
"This software is provided as-is.  The author can not be held "\
"liable for any damage that might arise from the use of this "\
"software.  Use it at your own risk.\n"\
"\n"\
"COPYRIGHT\n"\
"=========\n"\
"\n"\
"This license covers the entirety of the PM123 source file distribution, "\
"except GBM which is left into the public domain, and the plug-ins."\
"\n"\
"Copyright 2006-2011 Marcel Mueller\n"\
"Copyright 2004-2007 Dmitry A.Steklenev\n"\
"Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>\n"\
"                    Taneli Lepp� <rosmo@sektori.com>\n"\
"\n"\
"Redistribution and use in source and binary forms, with or without "\
"modification, are permitted provided that the following conditions are met:\n"\
"\n"\
"   1. Redistributions of source code must retain the above copyright notice, "\
"      this list of conditions and the following disclaimer.\n"\
"\n"\
"   2. Redistributions in binary form must reproduce the above copyright "\
"      notice, this list of conditions and the following disclaimer in the "\
"      documentation and/or other materials provided with the distribution.\n"\
"\n"\
"   3. The name of the author may not be used to endorse or promote products "\
"      derived from this software without specific prior written permission.\n"\
"\n"\
"THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED "\
"WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF "\
"MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO "\
"EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, "\
"SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, "\
"PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR "\
"BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER "\
"IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) "\
"ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE "\
"POSSIBILITY OF SUCH DAMAGE."
#endif

#endif /* PM123_COPYRIGHT_H */
