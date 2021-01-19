
#ifndef DIRWRAPPER_H
#define DIRWRAPPER_H

/**
 * A class providing remote directory access services.
 * An instance of this class is to be used on a client or a server system, connected to a matching
 * instance on a remote host.  It performs such operations as scanning the contents of a directory
 * and obtaining information about the files and directories contained therein, allowing fast remote
 * parsing of file systems without needing said file systems to be mounted as network file systems.
 */

class RDirWrapper
{
public:

	void close();
};

#endif /* ! DIRWRAPPER_H */
