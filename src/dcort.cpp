#include "dcort.h"
using namespace Rcpp;


// Function to calculate Astar
NumericMatrix Astar(NumericMatrix d) {
  int n = d.nrow();
  NumericVector m(n);
  double M = 0.0;

  // Calculate row means
  for (int i = 0; i < n; ++i) {
    double row_sum = 0.0;
    for (int j = 0; j < n; ++j) {
      row_sum += d(i, j);
    }
    m[i] = row_sum / n;
  }

  // Calculate overall mean
  for (int i = 0; i < n; ++i) {
    M += m[i];
  }
  M /= n;

  // Calculate Astar matrix
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      d(i, j) = d(i, j) - m[i] - m[j] + M - d(i,j) / n;
    }
  }

  // Apply correction
  for (int i = 0; i < n; ++i) {
    d(i, i) = m[i] - M;
  }
  
  d = n / (n - 1.0) * d;

  return d;
}

NumericMatrix dist(NumericMatrix data) {
  int n = data.nrow();
  NumericMatrix distances(n, n);
  
  for (int i = 0; i < n; ++i) {
    for (int j = i; j < n; ++j) {
      double distance = 0.0;
      for (int k = 0; k < data.ncol(); ++k) {
        distance += pow(data(i, k) - data(j, k), 2);
      }
      distances(i, j) = sqrt(distance);
      distances(j, i) = distances(i, j); // Distance matrix is symmetric
    }
  }
  
  return distances;
}

// Function to calculate bias corrected distance correlation
List bcdcor(const NumericMatrix& x, const NumericMatrix& y) {
  int n = x.nrow();

  // Compute pairwise distances for x and y
  NumericMatrix xDist = dist(x);
  NumericMatrix yDist = dist(y);
  
  // Compute Astar for x and y
  NumericMatrix AA = Astar(xDist);
  NumericMatrix BB = Astar(yDist);
  
  // Extract diagonal elements as vectors
  NumericVector diag_AA = diag(AA);
  NumericVector diag_BB = diag(BB);
  
  // Compute bcdcor components
  double XY = sum(AA * BB) - (n / (n - 2)) * sum(diag_AA * diag_BB);
  double XX = sum(AA * AA) - (n / (n - 2)) * sum(pow(diag_AA, 2));
  double YY = sum(BB * BB) - (n / (n - 2)) * sum(pow(diag_BB, 2));
  double bcR = XY / sqrt(XX * YY);
  
  // Return results as a list
  return List::create(Named("bcR") = bcR,
                      Named("XY") = XY / (n * n),
                      Named("XX") = XX / (n * n),
                      Named("YY") = YY / (n * n),
                      Named("n") = n);
}

// Function to calculate the t statistic for corrected high-dim dCor
double dcort(const NumericMatrix& x, const NumericMatrix& y) {
  List r = bcdcor(x, y);
  double Cn = as<double>(r["bcR"]);
  int n = as<int>(r["n"]);
  double M = n * (n - 3) / 2;
  return sqrt(M - 1) * Cn / sqrt(1 - pow(Cn, 2));
}

// Function to perform the dcor t-test of independence for high dimension
List dcort_test(const NumericMatrix& x, const NumericMatrix& y) {
  List stats = bcdcor(x, y);
  double bcR = as<double>(stats["bcR"]);
  int n = as<int>(stats["n"]);
  double M = n * (n - 3) / 2;
  int df = M - 1;
  double tstat = sqrt(M - 1) * bcR / sqrt(1 - pow(bcR, 2));  
  double pval = 1 - R::pt(tstat, df, 1, 0);
  return List::create(Named("statistic") = tstat,
                      Named("parameter") = df,
                      Named("p.value") = pval,
                      Named("estimate") = bcR,
                      Named("method") = "dcor t-test of independence for high dimension",
                      Named("data.name") = "x and y");
}


