/*----------------------------------------------------------------------------
 *
 *   Copyright (C) 2016 - 2018 Antonio Augusto Alves Junior
 *
 *   This file is part of Hydra Data Analysis Framework.
 *
 *   Hydra is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Hydra is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Hydra.  If not, see <http://www.gnu.org/licenses/>.
 *
 *---------------------------------------------------------------------------*/


/*
 * convolute_functions.inl
 *
 *  Created on: Nov 22, 2018
 *      Author: Antonio Augusto Alves Junior
 */

#ifndef CONVOLUTE_FUNCTIONS_INL_
#define CONVOLUTE_FUNCTIONS_INL_



#include <iostream>
#include <assert.h>
#include <time.h>
#include <chrono>
#include <vector>

//hydra
#include <hydra/Convolution.h>
#include <hydra/functions/Gaussian.h>
#include <hydra/functions/Ipatia.h>

//command line
#include <tclap/CmdLine.h>

/*-------------------------------------
 * Include classes from ROOT to fill
 * and draw histograms and plots.
 *-------------------------------------
 */
#ifdef _ROOT_AVAILABLE_

#include <TROOT.h>
#include <TH1D.h>
#include <TApplication.h>
#include <TCanvas.h>

#endif //_ROOT_AVAILABLE_


int main(int argv, char** argc)
{
	size_t nentries = 0;

	try {

		TCLAP::CmdLine cmd("Command line arguments for ", '=');

		TCLAP::ValueArg<size_t> EArg("n", "number-of-events","Number of events", true, 10e6, "size_t");
		cmd.add(EArg);

		// Parse the argv array.
		cmd.parse(argv, argc);

		// Get the value parsed by each arg.
		nentries = EArg.getValue();

	}
	catch (TCLAP::ArgException &e)  {
		std::cerr << "error: " << e.error() << " for arg " << e.argId()
														<< std::endl;
	}

	//-----------------
	// some definitions

	double min=423.677;;
	double max=580.677;

	auto nsamples = nentries;
	//===========================
	// kernels
	//---------------------------

	// gaussian
	auto mean   = hydra::Parameter::Create( "mean").Value(0.0).Error(0.0001);
	auto sigma  = hydra::Parameter::Create("sigma").Value(0.03).Error(0.0001);

	hydra::Gaussian<> gaussian_kernel(mean,  sigma);

	//===========================
	// signals
	//---------------------------


	//Ipatia
	//core
	auto mu    = hydra::Parameter::Create("mu").Value(493.677).Error(0.0001);
	auto gamma = hydra::Parameter::Create("sigma").Value(17.5).Error(0.0001);
	//left tail
	auto L1    = hydra::Parameter::Create("L1").Value(0.199).Error(0.0001); // decay speed
	auto N1    = hydra::Parameter::Create("N1").Value(14.0).Error(0.0001); // tail deepness
	//right tail
	auto L2    = hydra::Parameter::Create("L2").Value(0.62).Error(0.0001);// decay speed
	auto N2    = hydra::Parameter::Create("N2").Value(0.5).Error(0.0001);// tail deepness
	//peakness
	auto alfa  = hydra::Parameter::Create("alfa").Value(-1.01).Error(0.0001);
	auto beta  = hydra::Parameter::Create("beta").Value(-0.03).Error(0.0001);


	//ipatia function evaluating on the first argument
	auto signal = hydra::Ipatia<>(mu, gamma,L1,N1,L2,N2,alfa,beta);


	//===========================
	// samples
	//---------------------------
	std::vector<double> conv_result(nsamples, 0.0);

	//uniform (x) gaussian kernel

	auto start_d = std::chrono::high_resolution_clock::now();

	hydra::convolute(signal, gaussian_kernel, min, max,  conv_result );

	auto end_d = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double, std::milli> elapsed_d = end_d - start_d;
	//time
	std::cout << "-----------------------------------------"<<std::endl;
	std::cout << "| Time (ms) ="<< elapsed_d.count()    <<std::endl;
	std::cout << "-----------------------------------------"<<std::endl;



	//------------------------
	//------------------------
#ifdef _ROOT_AVAILABLE_
	//fill histograms
	TH1D *hist_convol   = new TH1D("convol","convolution", nsamples+1, min, max);
	TH1D *hist_signal   = new TH1D("signal", "signal", nsamples+1, min, max);
	TH1D *hist_kernel   = new TH1D("kernel", "kernel", nsamples+1, min, max);


	for(size_t i=1;  i<nsamples+1; i++){
		double x =hist_convol->GetBinCenter(i);

		hist_convol->SetBinContent(i, conv_result[i-1] );
		hist_signal->SetBinContent(i, signal(x) );
		hist_kernel->SetBinContent(i, gaussian_kernel((max-min)/2-i*(hist_convol->GetBinWidth(0))) );


	}
#endif //_ROOT_AVAILABLE_






#ifdef _ROOT_AVAILABLE_
	TApplication *myapp=new TApplication("myapp",0,0);

	//----------------------------
	//draw histograms
	TCanvas* canvas = new TCanvas("canvas" ,"canvas", 1000, 1000);
	canvas->Divide(2,2);

	canvas->cd(1);
	hist_convol->SetStats(0);
	hist_convol->SetLineColor(3);
	hist_convol->SetLineWidth(1);
	hist_convol->Draw("histl");

	canvas->cd(2);
	hist_signal->SetStats(0);
	hist_signal->SetLineColor(2);
	hist_signal->SetLineWidth(1);
	hist_signal->Draw("histl");

	canvas->cd(3);
	hist_kernel->SetStats(0);
	hist_kernel->SetLineColor(5);
	hist_kernel->SetLineWidth(2);
	hist_kernel->Draw("histl");

	canvas->cd(4);
	hist_convol->DrawNormalized("histl");
	hist_signal->DrawNormalized("histlsame");



	myapp->Run();

#endif //_ROOT_AVAILABLE_

	return 0;

}


#endif /* CONVOLUTE_FUNCTIONS_INL_ */
