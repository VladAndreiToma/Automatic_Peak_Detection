/*

#################################################################################
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>  // Include filesystem for file iteration
#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TCanvas.h"
#include "TSpectrum.h"

namespace fs = std::filesystem;  // Use the filesystem namespace

using namespace std;

ofstream fout("logs.log");

// Function to read the ROOT file and extract the TTree
TTree* ReadTheFile(const char* fileName) {
    TFile* reader = TFile::Open(fileName);
    if (!reader || reader->IsZombie()) {
        cerr << "Error: Unable to open file " << fileName << endl;
        return nullptr;
    }
    TTree* myTree = dynamic_cast<TTree*>(reader->Get("data"));
    if (!myTree) {
        cerr << "Error: TTree 'data' not found in file " << fileName << endl;
        return nullptr;
    }
    return myTree;
}

// Function to fill a histogram from the TTree based on ChargeLong values
std::vector<TH1F*> formHistos(TTree* tree, UShort_t* cLong, UChar_t* channel, UChar_t* mod) {
    std::vector<TH1F*> histograms;
    histograms.push_back(new TH1F("longCharge_ch0", "LongCharge_ch0", 2000, 0, 2000));
    histograms.push_back(new TH1F("longCharge_ch1", "LongCharge_ch1", 2000, 0, 2000));
    histograms.push_back(new TH1F("longCharge_ch2", "LongCharge_ch2", 2000, 0, 2000));
    histograms.push_back(new TH1F("longCharge_ch3", "LongCharge_ch3", 2000, 0, 2000));
    
    int nEntries = tree->GetEntries();
    
    // Loop over entries and fill histograms
    for (int index = 0; index < nEntries; ++index) {
        tree->GetEntry(index);  // Load the tree entry
        switch (*channel) {
            case 0:
                histograms[0]->Fill(*cLong);
                break;
            case 1:
                histograms[1]->Fill(*cLong);
                break;
            case 2:
                histograms[2]->Fill(*cLong);
                break;
            case 3:
                histograms[3]->Fill(*cLong);
                break;
        }
    }
    return histograms;
}

// Function to draw histograms in separate pads of the same canvas
void draw_Histograms(std::vector<TH1F*> histos, const std::string& fileName) {
    TCanvas* myCanvas = new TCanvas(fileName.c_str(), ("Plot details for " + fileName).c_str());
    myCanvas->Divide(2, 2); // Divide canvas into 2x2 pads

    // Loop through histograms and draw each in a separate pad
    for (size_t i = 0; i < histos.size(); ++i) {
        myCanvas->cd(i + 1);  // Go to the i-th pad
        histos[i]->SetLineColor(i + 1);  // Set line color for distinction
        histos[i]->SetLineWidth(2);
        histos[i]->Draw("HIST");
    }

    myCanvas->Update(); // Update the canvas to display the drawings
}

// Making analysis with TSpectrum
std::vector<TH1F*> Extract_Background_With_TSpectrum(std::vector<TH1F*> histos, Int_t iterations) {
    std::vector<TH1F*> bgExtracted_histos;
    TSpectrum *spectrum = new TSpectrum();
    
    for (auto &histogram : histos) {
        TH1 *background = spectrum->Background(histogram, iterations);
        TH1F* copyHist = (TH1F*) histogram->Clone();  // Use Clone() instead of Copy()
        copyHist->Add(background, -1);
        bgExtracted_histos.push_back(copyHist);
    }
    return bgExtracted_histos;
}

// Searching for peaks
void Search_For_Peaks(TH1F* histo, Double_t DEFAULT_GAUSSIAN_SPREAD, const std::string& fileName) {
    Double_t threshold = 0.5;
    Double_t sigma = 30.5;
    TSpectrum *spectrum = new TSpectrum();
    Int_t nPeaks = spectrum->Search(histo, sigma, "", threshold);
    Double_t *xPeaks = spectrum->GetPositionX();
    Double_t *yPeaks = spectrum->GetPositionY();
    
    fout << "File: " << fileName << " | Number of peaks found: " << nPeaks << std::endl;

    for (Int_t i = 0; i < nPeaks; i++) {
        fout << "File: " << fileName << " | Peak " << i + 1 << ": X = " << xPeaks[i] << ", Y = " << yPeaks[i] << std::endl;
    }

    // Fit Gaussian around each detected peak
    for (Int_t i = nPeaks - 1; i >= 0; i--) {
        Double_t peakPosition = xPeaks[i];  // X position of the peak
        Double_t fitRange = DEFAULT_GAUSSIAN_SPREAD;

        TF1 *gaussian = new TF1("gaus", "gaus", peakPosition - fitRange, peakPosition + fitRange);
        histo->Fit(gaussian, "R");

        // Extract mean and sigma from the fit
        Double_t mean = gaussian->GetParameter(1);  // Mean (position of peak)
        Double_t sigma = gaussian->GetParameter(2);  // Sigma (width of the peak)

        // Output the results
        fout << "File: " << fileName << " | Peak " << i + 1 << ": Mean = " << mean << ", Sigma = " << sigma 
             << " / Energy resolution: " << (2.54 * sigma / mean) * ((i == 1) ? 1117 : 1332) << " keV" << std::endl;
    }
}

void analysis() {
    // 1. Define the raw data directory
    std::string rawDataDir = "./rawData/";
    int filesProcessed = 0;

    // 2. Iterate over files in the raw data directory
    for (const auto& entry : fs::directory_iterator(rawDataDir)) {
        if (filesProcessed >= 50) break;  // Stop after processing 50 files
        if (entry.path().extension() == ".root") {
            std::string fileName = entry.path().filename().string();
            fout << "Processing file: " << fileName << std::endl;

            // 3. Read the ROOT file
            TTree* dataTree = ReadTheFile(entry.path().c_str());
            if (!dataTree) continue;

            // 4. Set branch addresses
            UChar_t Mod, Ch;
            Double_t FineTS;
            UShort_t ChargeLong;
            Short_t ChargeShort;
            vector<Short_t> Signal;

            dataTree->SetBranchAddress("Mod", &Mod);
            dataTree->SetBranchAddress("Ch", &Ch);
            dataTree->SetBranchAddress("FineTS", &FineTS);
            dataTree->SetBranchAddress("ChargeLong", &ChargeLong);
            dataTree->SetBranchAddress("ChargeShort", &ChargeShort);
            dataTree->SetBranchAddress("Signal", &Signal);

            // 5. Create the array of histos separated on channels
            std::vector<TH1F*> expHistos = formHistos(dataTree, &ChargeLong, &Ch, &Mod);

            // 6. Extract the background
            std::vector<TH1F*> bg_Extracted_expHistos = Extract_Background_With_TSpectrum(expHistos, 20);

            // 7. Draw histograms
            draw_Histograms(bg_Extracted_expHistos, fileName);

            // 8. Search for peaks in each background-extracted histogram
            Double_t Co60_SPREAD = 30;
            for (TH1F* histo : bg_Extracted_expHistos) {
                Search_For_Peaks(histo, Co60_SPREAD, fileName);
            }

            filesProcessed++;
        }
    }
}

int main() {
    analysis();
    return 0;
}
#################################################################################################################
*/














#include <iostream>
#include <fstream>
#include <vector>
#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TCanvas.h"
#include "TSpectrum.h"

using namespace std;

ofstream fout("logs.log");

// Function to read the ROOT file and extract the TTree
TTree* ReadTheFile(const char* fileName) {
    TFile* reader = TFile::Open(fileName);
    if (!reader || reader->IsZombie()) {
        cerr << "Error: Unable to open file " << fileName << endl;
        return nullptr;
    }
    TTree* myTree = dynamic_cast<TTree*>(reader->Get("data"));
    if (!myTree) {
        cerr << "Error: TTree 'data' not found in file " << fileName << endl;
        return nullptr;
    }
    return myTree;
}

// Function to fill a histogram from the TTree based on ChargeLong values
std::vector<TH1F*> formHistos(TTree* tree, UShort_t* cLong, UChar_t* channel, UChar_t* mod) {
    std::vector<TH1F*> histograms;
    histograms.push_back(new TH1F("longCharge_ch0", "LongCharge_ch0", 2000, 0, 2000));
    histograms.push_back(new TH1F("longCharge_ch1", "LongCharge_ch1", 2000, 0, 2000));
    histograms.push_back(new TH1F("longCharge_ch2", "LongCharge_ch2", 2000, 0, 2000));
    histograms.push_back(new TH1F("longCharge_ch3", "LongCharge_ch3", 2000, 0, 2000));
    
    int nEntries = tree->GetEntries();
    
    // Loop over entries and fill histograms
    for (int index = 0; index < nEntries; ++index) {
        tree->GetEntry(index);  // Load the tree entry
        switch (*channel) {
            case 0:
                histograms[0]->Fill(*cLong);
                break;
            case 1:
                histograms[1]->Fill(*cLong);
                break;
            case 2:
                histograms[2]->Fill(*cLong);
                break;
            case 3:
                histograms[3]->Fill(*cLong);
                break;
        }
    }
    return histograms;
}

// Function to draw histograms in separate pads of the same canvas
void draw_Histograms(std::vector<TH1F*> histos) {
    TCanvas* myCanvas = new TCanvas("myCanvas", "Plot details Co60");
    myCanvas->Divide(2, 2); // Divide canvas into 2x2 pads

    // Loop through histograms and draw each in a separate pad
    for (size_t i = 0; i < histos.size(); ++i) {
        myCanvas->cd(i + 1);  // Go to the i-th pad
        histos[i]->SetLineColor(i + 1);  // Set line color for distinction
        histos[i]->SetLineWidth(2);
        histos[i]->Draw("HIST");
    }

    myCanvas->Update(); // Update the canvas to display the drawings
}

// Making analysis with TSpectrum
std::vector<TH1F*> Extract_Background_With_TSpectrum(std::vector<TH1F*> histos, Int_t iterations) {
    std::vector<TH1F*> bgExtracted_histos;
    TSpectrum *spectrum = new TSpectrum();
    
    for (auto &histogram : histos) {
        TH1 *background = spectrum->Background(histogram, iterations);
        TH1F* copyHist = (TH1F*) histogram->Clone();  // Use Clone() instead of Copy()
        copyHist->Add(background, -1);
        bgExtracted_histos.push_back(copyHist);
    }
    return bgExtracted_histos;
}

// Searching for peaks
void Search_For_Peaks(TH1F* histo, Double_t DEFAULT_GAUSSIAN_SPREAD) {
    Double_t threshold = 0.5;
    Double_t sigma = 30.5;
    TSpectrum *spectrum = new TSpectrum();
    Int_t nPeaks = spectrum->Search(histo, sigma, "", threshold);
    Double_t *xPeaks = spectrum->GetPositionX();
    Double_t *yPeaks = spectrum->GetPositionY();
    fout << "Number of peaks found: " << nPeaks << std::endl;

    for (Int_t i = 0; i < nPeaks; i++) {
        fout << "Peak " << i + 1 << ": X = " << xPeaks[i] << ", Y = " << yPeaks[i] << std::endl;
    }

    // Fit Gaussian around each detected peak
    for (Int_t i = nPeaks - 1; i >= 0; i--) {
        Double_t peakPosition = xPeaks[i];  // X position of the peak
        Double_t fitRange = DEFAULT_GAUSSIAN_SPREAD;

        TF1 *gaussian = new TF1("gaus", "gaus", peakPosition - fitRange, peakPosition + fitRange);
        histo->Fit(gaussian, "R");

        // Extract mean and sigma from the fit
        Double_t mean = gaussian->GetParameter(1);  // Mean (position of peak)
        Double_t sigma = gaussian->GetParameter(2);  // Sigma (width of the peak)

        // Output the results
        fout << "Peak " << i + 1 << ": Mean = " << mean << ", Sigma = " << sigma 
             << " / Energy resolution: " << (2.54 * sigma / mean) * ((i == 1) ? 1117 : 1332) << " keV" << std::endl;
    }
}

void analysis() {
    // 1. Read the ROOT file
    TTree* dataTree = ReadTheFile("./rawData/run_500_60_4_CFD_SMOOTH_EXP_2_CFD_FRACTLIST_50_0.root");
    if (!dataTree) return;

    // 2. Set branch addresses
    UChar_t Mod, Ch;
    Double_t FineTS;
    UShort_t ChargeLong;
    Short_t ChargeShort;
    vector<Short_t> Signal;

    dataTree->SetBranchAddress("Mod", &Mod);
    dataTree->SetBranchAddress("Ch", &Ch);
    dataTree->SetBranchAddress("FineTS", &FineTS);
    dataTree->SetBranchAddress("ChargeLong", &ChargeLong);
    dataTree->SetBranchAddress("ChargeShort", &ChargeShort);
    dataTree->SetBranchAddress("Signal", &Signal);

    // 3. Create the array of histos separated on channels
    std::vector<TH1F*> expHistos = formHistos(dataTree, &ChargeLong, &Ch, &Mod);

    // 4. Extract the background
    std::vector<TH1F*> bg_Extracted_expHistos = Extract_Background_With_TSpectrum(expHistos, 20);

    // 5. Draw histograms
    draw_Histograms(bg_Extracted_expHistos);

    // 6. Search for peaks in each background-extracted histogram
    Double_t Co60_SPREAD = 30;
    for (TH1F* histo : bg_Extracted_expHistos) {
        Search_For_Peaks(histo, Co60_SPREAD);
    }
}




/*

    fout << "Tree of experiment is object: " << dataTree << endl;

    int APROX_MEAN_PEAK_1 = 910;
    int APROX_MEAN_PEAK_2 = 1030;
    int PEAK1_LEFT = 880;
    int PEAK1_RIGHT = 940;
    int PEAK2_LEFT = 1000;
    int PEAK2_RIGHT = 1070;

    TF1* gaus1 = new TF1( "gauss1" , "gaus" , PEAK1_LEFT , PEAK1_RIGHT );
    TF1* gaus2 = new TF1( "gauss2" , "gaus"  , PEAK2_LEFT , PEAK2_RIGHT );

h_longCharge -> Fit( gaus1 , "R" , "" , PEAK1_LEFT , PEAK1_RIGHT );
    h_longCharge -> Fit( gaus2 , "R" , "" , PEAK2_LEFT , PEAK2_RIGHT );
    

    // 4. Create a canvas and draw the histogram
    TCanvas* c1 = new TCanvas( "c1" , "ChargeLong Histogram" , 800 , 600 );
    h_longCharge -> Draw( "HIST" );
    gaus1 -> Draw( "SAME" );
    gaus2 -> Draw( "SAME" );

    // Save canvas to a file
    c1 -> SaveAs( "ChargeLong_Histogram.png" );
 */