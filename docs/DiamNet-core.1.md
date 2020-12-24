% DiamNet-core(1)
% DiamNet Development Foundation
%

# NAME

DiamNet-core - Core daemon for DiamNet payment network

# SYNOPSYS

DiamNet-core [OPTIONS]

# DESCRIPTION

DiamNet is a decentralized, federated peer-to-peer network that allows
people to send payments in any asset anywhere in the world
instantaneously, and with minimal fee. `DiamNet-core` is the core
component of this network. `DiamNet-core` is a C++ implementation of
the DiamNet Consensus Protocol configured to construct a chain of
ledgers that are guaranteed to be in agreement across all the
participating nodes at all times.

## Configuration file

In most modes of operation, DiamNet-core requires a configuration
file.  By default, it looks for a file called `DiamNet-core.cfg` in
the current working directory, but this default can be changed by the
`--conf` command-line option.  The configuration file is in TOML
syntax.  The full set of supported directives can be found in
`%prefix%/share/doc/DiamNet-core_example.cfg`.

%commands%

# EXAMPLES

See `%prefix%/share/doc/*.cfg` for some example DiamNet-core
configuration files

# FILES

DiamNet-core.cfg
:   Configuration file (in current working directory by default)

# SEE ALSO

<https://www.DiamNet.org/developers/DiamNet-core/software/admin.html>
:   DiamNet-core administration guide

<https://www.DiamNet.org>
:   Home page of DiamNet development foundation

# BUGS

Please report bugs using the github issue tracker:\
<https://github.com/DiamNet/DiamNet-core/issues>
