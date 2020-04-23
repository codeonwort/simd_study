# simd_study
Quick test after reading "SIMD at Insomniac Games."

Result on my computer:

* Debug build
	* Scalar(AOS): 0.7304680 seconds
	* Scalar(SOA): 0.0258607 seconds (28.24x faster) ...what?
	* SSE2       : 0.0589321 seconds (12.39x faster)
	* AVX2       : 0.0312854 seconds (23.34x faster)
* Release build
	* Scalar(AOS): 0.0429811 seconds
	* Scalar(SOA): 0.0040337 seconds (10.65x faster)
	* SSE2       : 0.0015692 seconds (27.39x faster)
	* AVX2       : 0.0006341 seconds (67.78x faster)

# References
* video: https://www.gdcvault.com/play/1022248/SIMD-at-Insomniac-Games-How
* pdf: https://deplinenoise.files.wordpress.com/2015/03/gdc2015_afredriksson_simd.pdf
