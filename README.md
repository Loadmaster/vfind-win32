# VFind-Win32
File finder - Command-line utility - Windows (Win32)<br/>
Find matching filenames in a directory tree.<br/>

<pre>
[<b>vfind</b>, 6.2 2016-01-18]

usage:  <b>vfind</b> [<i>option</i>...] [<i>path</i>\]<i>file</i>...

Options:
    <b>-?</b>          Show information about this program.

    <b>-a</b>          Print all matching entries.

    <b>-A</b>          Print all matching entries except "." and "..".

    <b>-d</b>[<b>+</b>|<b>-</b>|<b>!</b>]<i>D</i>  Find files modified [after|before|not] date <i>D</i>, which is of
                the form "[<b>YY</b>]<b>YY</b>[-<b>MM</b>[-<b>DD</b>]][:<b>HH</b>[:<b>MM</b>[:<b>SS</b>]]]",
                or is "<b>now</b>" (the current time), "<b>today</b>" (00:00 today),
                "<b>yesterday</b>", or "<b>tomorrow</b>".
                Two options can be given to specify a range of dates.

    <b>-f</b>          Show filenames without drive or path prefixes.

    <b>-l</b>          Long listing.

    <b>-m</b>          Show short DOS names.

    <b>-n</b>          Show a list summary.

    <b>-r</b>          Do not recursively search subdirectories.

    <b>-s</b>[<b>+</b>|<b>-</b>|<b>!</b>]<i>N</i>  File size is [more|less|not] <i>N</i> bytes.
                <i>N</i> can have one of these suffixes:
                    <b>k</b>  Kilobytes (1,204 bytes)
                    <b>m</b>  Megabytes (1,048,576 bytes)
                    <b>g</b>  Gigabytes (1,073,741,824 bytes)
                Two options can be given to specify a lower and upper range of file sizes.

    <b>-t</b>[<b>!</b>]<i>T</i>      Find entries [not] of type <i>T</i>, which is one or more of these
                attributes combined (or-ed) together:
                    <b>a</b>  Archive          <b>f</b>  File             <b>s</b>  System
                    <b>b</b>  Device           <b>h</b>  Hidden           <b>v</b>  Virtual
                    <b>c</b>  Compressed       <b>l</b>  Volume label     <b>w</b>  Writable
                    <b>d</b>  Directory        <b>o</b>  Offline
                    <b>e</b>  Encrypted        <b>r</b>  Read only

    <b>-v</b>          Verbose output.

Filenames can contain wildcard characters:
    <b>?</b>           Matches any single character (including '.').
    <b>*</b>           Matches zero or more characters (including '.').
    <b>[</b>abc<b>]</b>       Matches 'a', 'b', or 'c'.
    <b>[</b>a<b>-</b>z<b>]</b>       Matches 'a' through 'z'.
    <b>[!</b>a<b>-</b>z<b>]</b>      Matches any character except 'a' thru 'z'.
    <b>`</b><i>X</i>          Matches <i>X</i> exactly (<i>X</i> can be a wildcard character).
    !<i>X</i>          Matches any filename except <i>X</i>.
</pre>
