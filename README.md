# `dfg`

`dfg` is a dotfile configuration utility.

`dfg` works by storing dotfiles in a single location and managing symlinks to
them. This makes it much easier to manage, backup, and configure your dotfiles
by keeping them all contained within a single directory.

## Usage

```
A dotfile configuration utility.

usage: dfg [-hfu] [-s <path>] [-r <root-path>] <profile|profile:path>...

options:
    -h          Display usage information.
    -f          Overwrite any existing files encountered.
    -u          Unlink the given profiles instead of linking them.
    -r          Path to the root directory for links. [default: $HOME]\n
    -s          Path to the profile store. [default: $HOME/.dfg]
```

## Configuration

If the `$DFG_STORE` directory is set, then `dfg` will use that as the path to
the profile store instead (it is equivalent to using the `-s` option).
