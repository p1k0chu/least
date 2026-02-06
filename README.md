# least

Least is a pager that does less than less. It does almost nothing - just
shows the data line by line. That's it. It does the least you expect
pager to do. It can't go back in files either.

## Building

This project uses [cbuild](https://github.com/p1k0chu/cbuild).  
Run script `./lib/cbuild/bootstrap.sh`, for initial compilation of
`build.c`, then run `./build` to build the project

## Usage

```
least FILE			-- page one file
least < FILE		-- page one file
command | least		-- page the output of command
```

Keys:
- `j` or arrow down: scroll down one line
- `d`: scroll down half of your screen
- `f`: scroll down by full screen
- `q`: quit

## License

Copyright (c) 2026 p1k0chu. All Rights Reserved.

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License v3 or later
(see COPYING)

