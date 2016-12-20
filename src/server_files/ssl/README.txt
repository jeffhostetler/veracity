The file in this directory is loaded only on Windows by Veracity clients whenever a network connection is created.  It contains the root certificate authority information used by libcurl to validate a certificate.  It is unneeded on Linux and OS X, since the system already has a list of root certificate authorities.

If you need to find a new copy of the root certificates file, see http://curl.haxx.se/docs/caextract.html
