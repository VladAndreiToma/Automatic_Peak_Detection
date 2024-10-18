This is a refurbished code variant for our principia to automatically detect peaks and extract backgrounds to perform resolution analysis for HPGe Detectors and LaBr3 detectors
We are using TSpectrum class which we enhanced in functionality to automatically detect peaks and extract background. Its a step problem, same algos are repetead N times
to ensure better data processing. In the end we calculate the resolution of spectra using (FWHM/centroid) * Energy_detected for some processes detected with our detectors.
