#include <iostream>

using namespace std;

int lookup[] = {0,  0,  1,  39, 2,  15, 40, 23, 3,  12, 16, 59, 41, 19,
                24, 54, 4,  0,  13, 10, 17, 62, 60, 28, 42, 30, 20, 51,
                25, 44, 55, 47, 5,  32, 0,  38, 14, 22, 11, 58, 18, 53,
                63, 9,  61, 27, 29, 50, 43, 46, 31, 37, 21, 57, 52, 8,
                26, 49, 45, 36, 56, 7,  48, 35, 6,  34};

int main() {
  // cout << __builtin_ctz(5) << endl;
  // cout << __builtin_ctz(256) << endl;

  for (int i = 0; i < 50; ++i) {
    // cout << __builtin_ctz((1 << i)) << endl;
    //    cout << i << ": " << (i & -i) << endl;
  }

  int ctzll[67] = {0};
  for (int i = 0; i < 64; ++i) {
    cout << i << " => " << (1ULL << i) % 67 << endl;
    ctzll[(1ULL << i) % 67] = i;
  }
  cout << "{";
  for (int i = 0; i < 66; ++i) {
    cout << ctzll[i] << ", ";
  }
  cout << ctzll[66] << "}" << endl;

  unsigned long long n;
  for (unsigned long long i = 0; i < 100000000; ++i, n = i) {
    const int expected = __builtin_ctzll(i);
    const int actual = lookup[(i & -i) % 67];
    if (expected != actual) {
      cout << "error at " << i << "  expected=" << expected
           << "  actual=" << actual << endl;
      cout << "  i & -i = " << (i & -i) << endl;
    }
  }
  cout << "tested " << n << endl;

  return 0;
}
