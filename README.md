------------------------------------------------------------------
                              Tweaked OpenCV                      
------------------------------------------------------------------

This project introduces ARM's NEON and Ne10 optimisations to
particular portions of the OpenCV library. Optimised sections
include:

	./core/include/opencv2/core/internal.hpp
	./core/src/convert.cpp
	./imgproc/src/filter.cpp
	./imgproc/src/thresh.cpp


existing (Itseez?) NEON optimisations for Hamming norms can be found here:

	./core/src/stat.cpp->normHamming
