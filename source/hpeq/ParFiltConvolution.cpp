#include "ParFiltConvolution.h"

#include "IRTools.h"
#include <algorithm>
#include <../libs/Eigen/Dense>
#include <fstream>

#include "JuceHeader.h"

ParFiltConvolution::ParFiltConvolution()
{
	auto roots = findRoots({1,0});

	DBG("Test");
}


void ParFiltConvolution::process(const float * readL, const float * readR, float * writeL, float * writeR, unsigned int numSamples)
{
}

void ParFiltConvolution::updateFilterBank(unsigned int order, float lambda)
{	
	auto ir = getIR();
	lambda = std::max(0.f, std::min(1.f, lambda));

	unsigned int numSecondOrderSections = 2 << order;

	numSecondOrderSections = std::min(numSecondOrderSections/2, ir->getSize());

	order = std::log2(numSecondOrderSections);

	auto preparedResponse = *getIR();
	
	IRTools::makeMinPhase(preparedResponse);
	auto warpedResponse = IRTools::warp(preparedResponse, lambda, ir->getSize());


	

}


void ParFiltConvolution::onImpulseResponseUpdated()
{
	updateFilterBank(this->order, this->lambda);
}




std::vector<std::complex<double>> ParFiltConvolution::findPoles(const ImpulseResponse & ir, unsigned int order)
{
	using namespace Eigen;

	using Mat = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;

	auto mat2str = [](const Mat &m) -> std::string
	{
		std::stringstream out;
		
		out << m;
		return out.str();
	};

	auto printVec = [](auto s, bool flush = false)
	{
		auto flags = flush ? std::ofstream::trunc : std::ofstream::app;
		std::ofstream out("D:\\Temp\\temp.txt", std::ofstream::out | flags);

		for (auto i : s)
		{
			out << i << ", ";
		}
		out.close();
	};


	auto print = [](auto s, bool flush = false)
	{
		auto flags = flush ? std::ofstream::trunc : std::ofstream::app;
		std::ofstream out("D:\\Temp\\temp.txt", std::ofstream::out | flags);

		out << s;
		out.close();
	};


	auto h = ir.getChannel(0);
	auto h0 = h[0];
	auto g0 =  (h[0] != 0) ? h[0] : 1;

	unsigned int N = 2 << order;
	unsigned int K = ir.getSize();


	Mat H, H1, H2, h1;
	H.resize(K, N+1);

	// compose Toeplitz matrix H and H1
	for (int c = 0; c < (N + 1); c++)
	{
		for (int r = 0; r < K; r++)
		{
			auto idx = r - c;
			idx = std::max(-1, std::min((int)K - 1, idx));
			H(r, c) = (idx >= 0) *  h[idx] / g0;
		}
	}
	
	H1 = H.block(0, 0, N + 1, N + 1);
	h1 = H.block(N+1, 0, K-(N+1), 1);
	H2 = H.block(N+1, 1, K-(N+1), N);

	print(mat2str(H1), true);	
	print(mat2str(h1), true);
	print(mat2str(H2), true);

	Mat aP = H2.fullPivHouseholderQr().solve(h1);
	aP.transposeInPlace();

	Mat a;
	a.resize(1, aP.cols() + 1);
	a << 1, -aP;
	
	Mat b = g0 * (a * H1.transpose());
	
	printVec(ir.getLeftVector(), true);
	print("\n");
	print("B = [" + mat2str(b) + "]; \n");
	print("A = [" + mat2str(a) + "];");
		
	return findRoots(std::vector<double>(a.data(), a.data() + a.size()));
}

std::pair<std::vector<double>, std::vector<double>> ParFiltConvolution::prony(std::vector<double> h, unsigned int order)
{
	using namespace Eigen;

	using Mat = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;

	auto h0 = h[0];
	auto g0 = (h[0] != 0) ? h[0] : 1;

	unsigned int N = 2 << order;
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
			H(r, c) = (idx >= 0) *  h[idx] / g0;
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

std::vector<std::complex<double>> ParFiltConvolution::findRoots(std::vector<double> polynom)
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
