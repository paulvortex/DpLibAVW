Darkplaces LibAv Wrapper
----

This little lib is created to provide ability to dynamically link libav to Darkplaces engine with no care of ABI.

Each libavw build is aimed to certain LibAv revision and should be supplied with DLL's from it (so it going to be 5 libs - libavw, libavcodec, libavformat, libavutil, swscale).

--------------------------------------------------------------------------------
 Version History + Changelog (Reverse Chronological Order)
--------------------------------------------------------------------------------

0.6 (05-04-2013)
------
- LibAv 9.5 support
- Fixed InputContext buffer allocation to use av_malloc()
- free InputContext before FormatContext get freed

0.5 (24-12-2012)
------
- initial version with support for LibAv GIT fd0b8d5 build