/*
 * InputParams.h
 *
 *  Created on: Oct 17, 2012
 *      Author: bmetcalf
 */


#ifndef INPUTPARAMS_H_
#define INPUTPARAMS_H_

/**
 * This class is for reading in and writing parameter files and storing the parameters to be passed around the code.
 *
 * In the parameter file comments can be added with the comment character is #.  Any information
 * on a line after a # is stripped out.
 *
 * A valid parameter must have a label that starts a new line followed by at least one space
 * and then the value of the parameter followed by a comment (with #) if desired.
 *
 * The get() methods are used to retrieve parameter values and test if they exist.
 */

#include "standard.h"

typedef enum {PS, ST, PL} MassFuncType;

typedef enum {null_lens, nfw_lens, pnfw_lens, pl_lens, nsie_lens, ana_lens, uni_lens, moka_lens, dummy_lens, hern_lens} LensHaloType;
typedef enum {null_gal, nsie_gal} GalaxyLensHaloType;

/// names of clump and sb models
typedef enum {nfw,powerlaw,pointmass} ClumpInternal;
enum IMFtype {One,Mono,BrokenPowerLaw,Salpeter,SinglePowerLaw,Kroupa,Chabrier};
enum Band {SDSS_U,SDSS_G,SDSS_R,SDSS_I,SDSS_Z,J,H,Ks,i1,i2};

/** \brief Structure for reading and writing parameters to and from a parameter file as well as a container 
 * for passing the parameters to other classes and functions.
 *
 *  The constructor reads in all parameters in the parameter file.  They can then be accessed with the 
 *  get functions.  There should be no interaction with the parameter file except through the InputParams 
 *  structure.
 */
class InputParams {
public:
	InputParams(std::string paramfile);
	~InputParams();

	bool get(std::string label,bool& value);
	bool get(std::string label,std::string& value);
	bool get(std::string label,MassFuncType& value);
	bool get(std::string label,LensHaloType& value);
	bool get(std::string label,GalaxyLensHaloType& value);
	bool get(std::string label,ClumpInternal& value);
	bool get(std::string label,Band& value);
	bool get(std::string label,IMFtype& value);

	// get numbers
	template<typename Number>
	bool get(std::string label, Number& value);

	bool exist(std::string label);
	void print();
	void print_used();
	void print_unused();
	/// Returns total number of parameters.
	unsigned int Nparams(){return labels.size();}
	void PrintToFile(std::string filename);
	void PrintUsedToFile(std::string filename);
	/// Return name of the parameter file.
	std::string filename(){return paramfile_name;}

	void put(std::string label,std::string value,std::string comment = "");
	
	template<typename Number>
	void put(std::string label,Number value,std::string comment = "");

private:
	std::string paramfile_name;

	std::vector<std::string> labels;
	std::vector<std::string> char_values;
	std::vector<std::string> comments;
	// The number of times a parameter was retrieved by get
	std::vector<int> use_number;
};

/** \brief Assigns to "value" the value of the parameter called "label".
 * If this parameter label does not appear in the parameter file false
 * is returned and the value is set to the default value of the numerical type.
 * 
 * The entry in the parameter file must have a numerical value.  If it
 * does not, an exception message is printed and false is returned.
 */
template<typename Number>
bool InputParams::get(std::string label, Number& value)
{
	std::size_t i = std::distance(labels.begin(), std::find(labels.begin(), labels.end(), label));
	if(i == labels.size())
	{
		value = Number();
		return false;
	}
	
	std::istringstream sstr(char_values[i]);
	
	sstr >> value;
	if(sstr.fail())
	{
		std::cout << "Expected a numerical value for " << labels[i] << " in parameter file " << paramfile_name << std::endl;
		return false;
	}
	
	use_number[i]++;
	
	return true;
}

/**
 * Add a new parameter to the parameter list.
 */
template<typename Number>
void InputParams::put(std::string label, Number value, std::string comment)
{
	labels.push_back(label);
	use_number.push_back(0);
	
	// convert to string
	std::ostringstream strs;
	strs << value;
	char_values.push_back(strs.str());
	if(comment == "") comments.push_back("");
	else comments.push_back("#" + comment);
}

#endif /* INPUTPARAMS_H_ */
