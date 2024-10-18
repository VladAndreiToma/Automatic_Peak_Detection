We have developed an enhanced variant of our code for performing automated peak detection and background extraction, aimed at conducting resolution analysis for High-Purity Germanium (HPGe) and Lanthanum Bromide (LaBr₃) detectors. Our approach leverages and extends the functionality of the TSpectrum class to improve the accuracy and reliability of peak detection and background subtraction in gamma-ray spectra.

The algorithm operates iteratively, applying peak detection and background extraction techniques multiple times (N iterations) to ensure robust data processing and refinement of spectral features. This iterative processing helps mitigate noise and enhances the clarity of detected peaks.

For resolution analysis, the Full Width at Half Maximum (FWHM) of each detected peak is calculated and normalized to the corresponding centroid. This is combined with the detected energy to compute the energy resolution, a critical parameter for evaluating the performance of the detectors in identifying distinct gamma-ray energies.

This method provides a systematic approach to improving spectral resolution and ensuring accurate measurement of gamma-ray events in various nuclear physics applications.

