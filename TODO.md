**TODO**

- add unit test suite
- create script for end-to-end testing harness
- fix implementation to pass end-to-end testing harness
- update `Makefile` to output library and link unit tests and executable
- add autotools `.configure` and `make install` steps
- move `libpng` and `expat` to package dependencies
- make debian package
- make macports package

**KNOWN ISSUES**

- some full-alpha shapes cut into zero-alpha areas
    - for full-alpha snake, concave smoothing for -slope on full-alpha start edge, and +slope on full-alpha finish edge, cut into zero-alpha sections
    - solution: retain dropped points in smoothing process, offset concave points on hard-alpha edges so that dropped points remain outside
- some alpha areas are truncated
    - for partial-alpha snake, convex apex is not preseved and trucates close to alpha pixels
    - solution: measure slope above and below point, mark as preserve if signs are different
- final pixel row is truncated
    - solution: provide virtual repeat of final row of pixels
- interleaved full-alpha strip within partial alpha becomes detached
    - solution: provide virtual continuation of partial alpha pixels along left and right edges of full-alpha
