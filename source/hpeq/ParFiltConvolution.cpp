#include "ParFiltConvolution.h"

#include "IRTools.h"
#include <algorithm>
#include <../libs/Eigen/Dense>
#include <fstream>

#include "JuceHeader.h"

ParFiltConvolution::ParFiltConvolution()
{
	onImpulseResponseUpdated();
}


void ParFiltConvolution::process(const float * readL, const float * readR, float * writeL, float * writeR, unsigned int numSamples)
{
	checkUpdatedFilterbank();

	if (onlineFilterBank == nullptr) return;

	for (int i = 0; i < numSamples; i++)
	{
		auto in = 0.5f * (readL[i] + readR[i]);
		float out = onlineFilterBank->b0 * in;
		for (auto &filter : onlineFilterBank->parallelFilters)
		{
			out += filter.tick(in);
		}
		writeL[i] = writeR[i] = out;
	}
}

void ParFiltConvolution::createNewFilterBank(unsigned int order, float lambda)
{	
	auto ir = getIR();
	auto preparedResponse = *getIR();

	unsigned int numSOSs = 2 << order;
	unsigned int iirOrder = 2 * numSOSs;

	lambda = std::max(0.f, std::min(1.f, lambda));
	
	// normalize
	IRTools::normalize(preparedResponse);

	// make min phase
	IRTools::makeMinPhase(preparedResponse);

	// mono support only right now
	IRTools::makeMono(preparedResponse);
	
	// warp the response
	preparedResponse = IRTools::warp(preparedResponse, lambda, ir->getSize());
	
	// FIR vetor
	std::vector<double> h(preparedResponse.getLeftVector().begin(), preparedResponse.getLeftVector().end());
	

	// prony iir approximation
	auto iir = prony(h, iirOrder);

	// find poles 
	auto poles = roots(iir.second);
	
	// unwarp poles
	for (auto & pole : poles) pole = (pole + std::complex<double>(lambda)) / (1. + std::complex<double>(lambda) * pole);

	// do the thing
	auto filterBank = approximateIR(h, poles, 1);

	// sync 
	{
		std::lock_guard<std::mutex>  syncLock(filterBankSyncMutex);

		this->offlineFilterBank = std::unique_ptr<FilterBank>(new FilterBank(filterBank));
		this->newFilterBankReady = true;
	}
}


void ParFiltConvolution::onImpulseResponseUpdated()
{
	// create new thread that generates the filter bank for us
	auto creatorThread = std::thread([=]()
	{
		this->createNewFilterBank(this->order, this->lambda); 
	});
	
	// let thread do its thing
	creatorThread.detach();
	
	return;
}

std::pair<std::vector<double>, std::vector<double>> ParFiltConvolution::prony(std::vector<double> h, unsigned int iirOrder)
{
	using namespace Eigen;
	using Mat = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;
	
	if (h.size() < iirOrder)
	{
		h.resize(iirOrder + 1);
	}

	auto h0 = h[0];
	auto g0 = (h[0] != 0) ? h[0] : 1;

	unsigned int N = iirOrder;
	unsigned int K = h.size();
	
	Mat H, H1, H2, h1;
	H.resize(K, N + 1);

	// compose Toeplitz matrix H and H1
	for (int c = 0; c < (N + 1); c++)
	{
		for (int r = 0; r < K; r++)
		{
			auto idx = r - c;
			idx = std::max(-1, std::min((int)K - 1, idx));
			H(r, c) = (idx >= 0) ? (h[idx] / g0) : 0;
		}
	}

	H1 = H.block(0, 0, N + 1, N + 1);
	h1 = H.block(N + 1, 0, K - (N + 1), 1);
	H2 = H.block(N + 1, 1, K - (N + 1), N);

	Mat aP = H2.fullPivHouseholderQr().solve(h1);
	aP.transposeInPlace();

	Mat a;
	a.resize(1, aP.cols() + 1);
	a << 1, -aP;

	Mat b = g0 * (a * H1.transpose());

	
	return std::pair<std::vector<double>, std::vector<double>>(
		std::vector<double>(b.data(), b.data() + b.size()),
		std::vector<double>(a.data(), a.data() + a.size()));
}

std::vector<std::complex<double>> ParFiltConvolution::roots(std::vector<double> polynom)
{
	using Mat = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;
	using Vec = Eigen::VectorXcd;

	std::vector<std::complex<double>> roots;

	// check if all are zero
	if (std::all_of(polynom.begin(), polynom.end(), [](double x) {return x == 0; }))
	{
		return roots;
	}

	// strip leading zeros
	auto firstNonZero = std::find_if(polynom.begin(), polynom.end(), [](double x) {return x != 0; });
	polynom = std::vector<double>(firstNonZero, polynom.end());

	// strip trailing zeros
	auto lastNonZero = std::find_if(polynom.rbegin(), polynom.rend(), [](double x) {return x != 0; });
	auto numTrailingZeros = std::distance(polynom.rbegin(), lastNonZero);

	polynom.resize(polynom.size() - numTrailingZeros);
	roots.resize(numTrailingZeros);

	if (polynom.size() == 0) return roots;

	// this removes leading elements that are small enough to create arithmetic issues
	auto normalizedPoly = std::vector<double>(polynom.begin()+1, polynom.end());
	while (std::any_of(normalizedPoly.begin(), normalizedPoly.end(), [](double x) {return isnan(x); }))
	{
		polynom = std::vector<double>(polynom.begin()+1, polynom.end());
		normalizedPoly = std::vector<double>(polynom.begin() + 1, polynom.end());
	}
	
	if (normalizedPoly.size() == 0) return roots;
	
	// create companion matrix
	Mat compMat;
	compMat.resize(normalizedPoly.size(), normalizedPoly.size());	
	compMat.fill(0);
	
	compMat(0, 0) = -normalizedPoly[0];
	for (int i = 0; i < (normalizedPoly.size()-1); i++)
	{
		compMat(0, i+1) = -normalizedPoly[i+1];
		compMat(i + 1, i) = 1;
	}

	Eigen::ComplexEigenSolver<Mat> solver(compMat);
	Vec result = solver.eigenvalues();

	// append to stuff we already found
	auto resultVec = std::vector<std::complex<double>>(result.data(), result.data() + result.size());
	roots.insert(roots.end(), resultVec.begin(), resultVec.end());
	
	return roots;
}

ParFiltConvolution::FilterBank ParFiltConvolution::approximateIR(std::vector<double> ir, std::vector<std::complex<double>> poles, unsigned int firOrder)
{
	using Mat = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;

	poles = sortPoles(poles);

	// matrix thingy
	Mat M;
	M.resize(ir.size(), poles.size() + firOrder);
	M.fill(0);

	// create artificial impulse responses
	unsigned int column = 0;
	for (int i = 0; i < (static_cast<int>(poles.size()) - 1); i += 2)
	{
		SOS<double> filter;
		auto a = poly({ poles[i], poles[i + 1] });
		filter.setCoeffs(1, 0, 0, a[1], a[2]);

		float out = filter.tick(1);
		for (int k = 0; k < (ir.size() - 1); k++)
		{
			M(k, column) = out;
			M(k + 1, column + 1) = out;
			out = filter.tick(0);
		}
		M(ir.size() - 1, column) = out;
		column += 2;
	}

	// first order element
	if (poles.size() % 2)
	{
		SOS<double> filter;
		auto a = poly({ std::complex<double>(poles.back()) });
		filter.setCoeffs(1, 0, 0, a[1], 0);

		float out = filter.tick(1);
		for (int k = 0; k < (ir.size()); k++)
		{
			M(k, column) = out;
			out = filter.tick(0);
		}
		column += 1;
	}

	// fir part
	for (int i = 0; i < firOrder; i++)
	{
		M(i, column) = 1;
		column += 1;
	}

	Mat y;
	y.resize(ir.size(), 1);
	for (int i = 0; i < ir.size(); i++) y(i, 0) = ir[i];

	Mat A = M.transpose() * M;
	Mat b = M.transpose() * y;

	Mat par = A.fullPivHouseholderQr().solve(b);


	std::vector<SOS<float>> secondOrderSections;


	for (int i = 0; i < (static_cast<int>(poles.size()) - 1); i += 2)
	{
		SOS<float>::Coeffs coeffs;
		auto a = poly({ poles[i], poles[i + 1] });
		coeffs.b0 = par(i, 0);
		coeffs.b1 = par(i + 1, 0);
		coeffs.b2 = 0;

		coeffs.a1 = a[1] / a[0];
		coeffs.a2 = a[2] / a[0];
		SOS<float> filter;
		filter.setCoeffs(coeffs);

		secondOrderSections.push_back(filter);
	}

	// first order element
	if (poles.size() % 2)
	{
		SOS<float>::Coeffs coeffs;
		auto a = poly({ poles.back() });
		coeffs.b0 = par(poles.size() - 1, 0);
		coeffs.a1 = a[1] / a[0];

		SOS<float> filter;
		filter.setCoeffs(coeffs);
		secondOrderSections.push_back(filter);
	}

	FilterBank fiterBank;
	fiterBank.parallelFilters = secondOrderSections;

	if (firOrder > 0)
	{
		fiterBank.b0 = par(poles.size());
	}

	// lets make a stability pass

	for (auto &sos : secondOrderSections)
	{
		auto iirPoles = roots({ 1, sos.getCoeffs().a1, sos.getCoeffs().a2 });

		for (auto pole : iirPoles)
		{
			assert(std::abs(pole) < 1.);			
		}
	}


	

	// ToDo: missing FIR part

	return fiterBank;
}

std::vector<double> ParFiltConvolution::poly(std::vector<std::complex<double>> roots)
{
	std::vector<std::complex<double>> c;
	c.resize(roots.size() + 1);

	c[0] = 1;


	for (int i = 0; i < roots.size(); i++)
	{
		for (int k = i + 1; k >= 1; k--)
		{
			c[k] -= roots[i] * c[k - 1];
		}
	}

	std::vector<double> realC;
	realC.resize(c.size());

	for (int i = 0; i < realC.size(); i++)
	{
		realC[i] = c[i].real();
	}

	return realC;
}

std::vector<std::complex<double>> ParFiltConvolution::sortPoles(std::vector<std::complex<double>> poles)
{
	// clean poles
	// 1. remove zeros or poles that are closer then -120dB at 0
	double zeroThreshold = std::pow(10., -120 / 20); // -120dB
	poles.erase(std::remove_if(poles.begin(), poles.end(), [=](std::complex<double> pole) {return std::abs(pole) < zeroThreshold; }), poles.end());

	// conj Poles only contains the positive pole, the conjugate is added later on
	std::vector<std::complex<double>> conjPoles;
	std::vector<double> realPoles;

	// 2. if poles are closer then zeroThreshold on real axis, we move them on the real axis
	// 3. we remove negative poles 
	for (auto pole : poles)
	{
		// flip poles outside of the unit circle
		if (std::abs(pole) >= 1) pole = 1. / std::conj(pole);

		// move border stable poles inside the unit circle slightly
		if (std::abs(pole) == 1) pole *= 0.9999;

		if (std::abs(pole.imag()) < zeroThreshold)
		{
			realPoles.push_back(pole.real());
		}
		else if (pole.imag() > 0)
		{
			conjPoles.push_back(pole);
		}
	}

	// sort back poles in one continous list

	poles.clear();
	poles.reserve(2 * conjPoles.size() + realPoles.size());

	for (auto conjPole : conjPoles)
	{
		poles.push_back(conjPole);
		poles.push_back(std::conj(conjPole));
	}
	for (auto realPoles : realPoles)
	{
		poles.push_back(realPoles);
	}

	return poles;
}

void ParFiltConvolution::checkUpdatedFilterbank()
{
	std::lock_guard<std::mutex> lock(filterBankSyncMutex);

	if (newFilterBankReady)
	{
		std::swap(onlineFilterBank, offlineFilterBank);
		newFilterBankReady = false;
	}
}
