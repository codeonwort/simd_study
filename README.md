# simd_study
Quick test after reading "SIMD at Insomniac Games."

Result on my computer:
* Debug build
	* lame: 0.7278700 seconds
	* SSE2: 0.0579500 seconds (12.56x faster)
	* AVX2: 0.0306839 seconds (23.72x faster)
* Release build
	* lame: 0.0435770 seconds
	* SSE2: 0.0015728 seconds (27.70x faster)
	* AVX2: 0.0006350 seconds (68.62x faster)

# References
* video: https://www.gdcvault.com/play/1022248/SIMD-at-Insomniac-Games-How
* pdf: https://deplinenoise.files.wordpress.com/2015/03/gdc2015_afredriksson_simd.pdf
