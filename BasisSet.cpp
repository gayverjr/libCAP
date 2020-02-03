#include <iostream>
#include <fstream>
#include <vector>
#include "BasisFunction.h"
#include "BasisSet.h"
#include <math.h>
#include "Atom.h"
#include <sstream>
#include <algorithm>
#include <string>
#include <map>
#include <list>
#include "Shell.h"
#include <locale>
#include <iterator>
#include <cassert>
# define M_PIl          3.141592653589793238462643383279502884
using namespace std;


BasisSet::BasisSet(std::string xyz_name,std::string basis_name)
{
	//carts = purecart;
	name = "user-specified";
	Nbasis = generateBasisFunctions(xyz_name,basis_name);
}

int BasisSet::generateBasisFunctions(std::string xyz_name,std::string basis_name)
{
	map<string, int> angmoms = {{"S", 0}, {"P", 1}, {"D", 2}};
	std::vector<Atom> geometry = read_xyz(xyz_name);
	map<string,std::vector<Shell>> basis_set=readBasis(basis_name);
	for (Atom atm: geometry)
	{
		std::vector<Shell> shells = basis_set[atm.element];
		for(Shell shell: shells)
		{
			int L=angmoms[shell.shell];
			//loop which determines order of cartesian functions :0
			int a=L; int b=0; int c=0;
			while (a>=0)
			{
				vector<int> shell_vec = {a,b,c};
				basis.push_back(BasisFunction(atm.coords,shell_vec,shell.exps,shell.coeffs));
				if (c<L-a)
				{
					b=b-1;
					c=c+1;
				}
				else
				{
					a=a-1;
					c=0;
					b=L-a;
				}
			}
		}
	}
	return basis.size();
}

map<string,std::vector<Shell>> BasisSet::readBasis(string basis_name)
{
    map<string,std::vector<Shell>> basis_set;
    std::ifstream is(basis_name);
    if (is.good())
    {
      std::string line, rest;
      while (std::getline(is, line) && line != "****") continue;
      bool nextelement = true, nextshell = false;
      std::string cur_element;
      std::vector<Shell> shells;
      // read lines till end
      while (std::getline(is, line))
      {
        // skipping empties and starting with '!' (the comment delimiter)
        if (line.empty() || line[0] == '!') continue;
        if (line == "****")
        {
          if(!shells.empty())
          {
        	  basis_set[cur_element]=shells;
          	  shells.clear();
          }
          nextelement = true;
          nextshell = false;
          continue;
        }
        if (nextelement)
        {
          nextelement = false;
          std::istringstream iss(line);
          iss >> cur_element >> rest;
          transform(cur_element.begin(),cur_element.end(),cur_element.begin(),::toupper);
          nextshell = true;
          continue;
        }
        if (nextshell)
        {
          std::istringstream iss(line);
          std::string shell_label;
          std::size_t n_prims;
          iss >> shell_label >> n_prims >> rest;
            vector<double> exps; vector<double> coeffs;
            for (size_t i=0; i<n_prims; i++)
            {
              while (std::getline(is, line) && (line.empty() || line[0] == '!')) continue;
              std::istringstream iss(line);
              double exponent, coefficient;
              iss >> exponent >> coefficient;
              exps.push_back(exponent);
              coeffs.push_back(coefficient);
            }
            shells.push_back(Shell(shell_label,exps,coeffs));
        }
      }
}
    return basis_set;
}
std::vector<Atom> BasisSet::read_xyz(std::string xyz_name)
{
	ifstream xyzfile;
	xyzfile.open(xyz_name);
	//read in number of atoms
	size_t natom;
	xyzfile >> natom;
	std::string rest_of_line;
	std::getline(xyzfile, rest_of_line);
	//read in comment
	std::string comment;
	std::getline(xyzfile, comment);
	//now the coordinates
	std::vector<Atom> atoms;
	for (size_t i=0;i<natom;i++)
	{
	    // read line
	    std::string linestr;
	    std::getline(xyzfile, linestr);
	    std::istringstream iss(linestr);
	    std::string element_symbol;
	    double x, y, z;
	    iss >> element_symbol >> x >> y >> z;
	    transform(element_symbol.begin(),element_symbol.end(),element_symbol.begin(),::toupper);
	    atoms.push_back(Atom(element_symbol,x,y,z));
	}
	return atoms;


}
