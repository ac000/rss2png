## Overview

Simple utility to take the first item from a rss feed and print the title
and the summary (upto 50 chars of each) to a transparent png that can then
be used in HTML email signatures etc.


## Building

Install some required packages

    $ sudo dnf install libcurl-devel cairo-devel gumbo-parser-devel dejavu-sans-fonts

Clone the repository and build it

    $ git clone https://github.com/ac000/rss2png
    $ cd rss2png
    $ make


## Running

    $ rss2png -f https://example.com/feed/ [-u https://example.com/]

or to get some debug output

    $ RSS2PNG_DEBUG=1 rss2png

The image will be placed under (can be overridden with the -o option)

    /var/tmp/rss.png


## License

rss2png is licensed under the MIT licence. See *MIT-LICENSE.txt*.


## Contributing

Patches and/or pull requests should be emailed to the project maintainer

    Andrew Clayton <andrew.clayton@securolytics.io>

preferably using *git-send-email(1)* and *git-request-pull(1)*

Code should follow the coding style as outlined in *CodingStyle*.

Also, code should be signed-off. This means adding a line that says

    Signed-off-by: Name <email>

at the end of each commit, indicating that you wrote the code and have the
right to pass it on as an open source patch.

See: <http://developercertificate.org/>
