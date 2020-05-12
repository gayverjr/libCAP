/*
 * read_qchem_fchk.cpp
 *
 *  Created on: Mar 12, 2020
 *      Author: JG
 */
#include <armadillo>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>
#include "qchem_interface.h"
#include "utils.h"

size_t total_TDMs_to_read(size_t nstates)
{
	size_t numTDMs = 0;
	for (size_t i=1;i<nstates;i++)
	{
		for (size_t j=i;j<nstates;j++)
			numTDMs++;
	}
	return numTDMs;
}

size_t get_TDM_start(size_t nstates, size_t state_idx)
{
	size_t tdm_idx = 2*nstates;
	for (size_t i=1;i<state_idx;i++)
	{
		for (size_t j=i;j<nstates;j++)
			tdm_idx+=2;
	}
	return tdm_idx;
}

//currently this is written for open shell systems which have alpha and beta densities
std::array<std::vector<std::vector<arma::mat>>,2> qchem_read_in_dms(std::string dmat_filename,size_t nstates, size_t num_bf)
{
	std::cout << "Reading file:" << dmat_filename << std::endl;
	std::vector<arma::mat> opdms;
	//start with state density matrices, alpha and beta densities
	std::ifstream is(dmat_filename);
    if (is.good())
    {
    	std::string line, rest;
    	std::getline(is, line);
    	while (opdms.size() < nstates*2)
    	{
				if (line.find("State Density")!= std::string::npos)
				{
					//last part of line should be number of elements to read
					size_t num_elements = stoi(split(line,' ').back());
					size_t lines_to_read = num_elements%5==0 ? (num_elements/5) : num_elements/5+1;
					std::vector<double> matrix_elements;
					for (size_t k=1;k<=lines_to_read;k++)
					{
						std::getline(is,line);
						std::vector<std::string> tokens = split(line,' ');
						for (auto token:tokens)
						{
							matrix_elements.push_back(std::stod(token));
						}
					}
					arma::mat st_opdm(num_bf,num_bf);
					st_opdm.zeros();
					fill_mat(matrix_elements,st_opdm);
					opdms.push_back(st_opdm);
				}
				else
					std::getline(is,line);
    	}
    	//now tdms for alpha and beta densities
    	while (opdms.size() < nstates*2 +total_TDMs_to_read(nstates)*2)
    	{
			if (line.find("Transition DM")!= std::string::npos)
			{
				//last part of line should be number of elements to read
				size_t num_elements = stoi(split(line,' ').back());
				size_t lines_to_read = num_elements%5==0 ? (num_elements/5) : num_elements/5+1;
				std::vector<double> matrix_elements;
				for (size_t k=1;k<=lines_to_read;k++)
				{
					std::getline(is,line);
					std::vector<std::string> tokens = split(line,' ');
					for (auto token:tokens)
						matrix_elements.push_back(std::stod(token));
				}
				arma::mat st_opdm(num_bf,num_bf);
				st_opdm.zeros();
				fill_mat(matrix_elements,st_opdm);
				opdms.push_back(st_opdm);
			}
			else
				std::getline(is,line);
    	}
    }
    //now that we have our density matrices, lets organize them into a
    //handy form which corresponds to how they'll actually be used
    std::vector<std::vector<arma::mat>> alpha_densities;
    std::vector<std::vector<arma::mat>> beta_densities;
    for (size_t state_idx=1; state_idx<=nstates;state_idx++)
    {
    	std::vector<arma::mat> alpha_row;
    	std::vector<arma::mat> beta_row;
    	//fill with placeholders
    	for (size_t i=1;i<state_idx;i++)
    	{
    		alpha_row.push_back(arma::mat(1,1));
    		beta_row.push_back(arma::mat(1,1));
    	}
    	//state densities
    	alpha_row.push_back(opdms[2*state_idx-2]);
    	beta_row.push_back(opdms[2*state_idx-1]);
    	//transition densities
    	size_t tdm_start = get_TDM_start(nstates,state_idx);
    	size_t tdm_end = tdm_start + 2*(nstates-state_idx);
    	for (size_t j=tdm_start;j<tdm_end;j+=2)
    	{
        	alpha_row.push_back(opdms[j]);
        	beta_row.push_back(opdms[j+1]);
    	}
    	alpha_densities.push_back(alpha_row);
    	beta_densities.push_back(beta_row);
    }
    //fill in the placeholders
    for (size_t i=0;i<nstates;i++)
    {
    	for(size_t j=0;j<i;j++)
    	{
    		alpha_densities[i][j]= alpha_densities[j][i];
    		beta_densities[i][j]= beta_densities[j][i];
    	}
    }
    return {alpha_densities,beta_densities};

}

arma::mat qchem_read_overlap(std::string dmat_filename, size_t num_bf)
{
    std::ifstream is(dmat_filename);
	arma::mat smat;
	smat.zeros(num_bf,num_bf);
    if (is.good())
    {
    	std::string line, rest;
    	while (line.find("Overlap Matrix")== std::string::npos)
        	std::getline(is, line);
    	size_t num_elements = stoi(split(line,' ').back());
		size_t lines_to_read = num_elements%5==0 ? (num_elements/5) : num_elements/5+1;
		std::vector<double> matrix_elements;
		for (size_t k=1;k<=lines_to_read;k++)
		{
			std::getline(is,line);
			std::vector<std::string> tokens = split(line,' ');
			for (auto token:tokens)
				matrix_elements.push_back(std::stod(token));
		}
		fill_LT(matrix_elements,smat);
    }
    return smat;
}

arma::mat read_qchem_energies(size_t nstates,std::string method,std::string output_file)
{
	std::cout << "Reading energies from file:" << output_file << std::endl;
	arma::mat ZERO_ORDER_H(nstates,nstates);
	ZERO_ORDER_H.zeros();
	transform(method.begin(),method.end(),method.begin(),::toupper);
	std::ifstream is(output_file);
    if (is.good())
    {
    	std::string line, rest;
    	std::getline(is, line);
    	size_t state_idx = 1;
    	while (state_idx<=nstates)
    	{
    		std::string line_to_find = method +" transition " + std::to_string(state_idx);
    		if (line.find(line_to_find)!= std::string::npos)
    		{
				std::getline(is,line);
				ZERO_ORDER_H(state_idx-1,state_idx-1) = std::stod(split(line,' ')[3]);
				state_idx++;
    		}
    		else
    			std::getline(is,line);
    	}
    }
    return ZERO_ORDER_H;
}
