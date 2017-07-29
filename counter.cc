#include <iostream>
#include <unistd.h>
#include <stdlib.h>

using namespace std;

int main(int argc, char** argv) {
  int n = 8;

  for(int i = 1; i <= n; i++) {
    cout << "Process:  " << getpid() << ' ' << i << endl;
  }

  return n;
}
