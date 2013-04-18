#ifndef MULTI_SOURCE_H_
#define MULTI_SOURCE_H_

#include <vector>
#include <map>
#include <cstddef>

#include "source.h"

class MultiSource : public Source
{
public:
	SOURCE_TYPE(MultiSource)
	
	/// Construct an empty MultiSource
	MultiSource();
	
	/// Create a MultiSource from input parameters.
	MultiSource(InputParams& params);
	
	/// Copy a MultiSource. The new MultiSource does not own any sources.
	MultiSource(const MultiSource& other);
	
	/// Destroy the MultiSource and free created sources.
	~MultiSource();
	
	/// Surface brightness of current source
	double SurfaceBrightness(double* y);
	
	/// Total flux coming from the current source
	double getTotalFlux();
	
	/// Get radius of current source.
	double getRadius();
	
	/// Return redshift of current source.
	double getZ();
	
	/// Set redshift of current source. Only changes the redshift while leaving position fixed.
	void setZ(double z);
	
	/// Return angular position of current source.
	double* getX();
	
	/// Set angular position of current source.
	void setX(double theta[2]);
	
	/// Set angular position of current source.
	void setX(double x1, double x2);
	
	/// Get number of sources.
	std::size_t size() const;
	
	/// Add a source. Ownership is not transferred to the MultiSource. Return the source index.
	std::size_t add(Source* source);
	
	/// Add a source. Ownership is not transferred to the MultiSource. Return the source index.
	std::size_t add(Source& source);
	
	/// Get the current source
	Source* getCurrent() const;
	
	/// Set the current source. Needs to be in list of sources. Does a costly find.
	bool setCurrent(Source* source);
	
	/// Get the current index
	std::size_t getIndex() const;
	
	/// Set the current index. Needs to be in range.
	bool setIndex(std::size_t i);
	
	/// Get current source in a given type. Returns 0 if types don't match.
	template<typename SourceT>
	SourceT* getCurrent() const
	{
		// use the SourceType to safely upcast the current Source*
		return source_cast<SourceT>(sources[index]);
	}
	
	/// Get the type of the current source.
	const SourceType getCurrentType() const
	{
		return sources[index]->type();
	}
	
	/// Get all sources.
	std::vector<Source*> getAll() const;
	
	/// Get all sources of a given type.
	std::vector<Source*> getAll(SourceType type) const;
	
	/// Get all sources of a given type in that type.
	template<typename SourceT>
	std::vector<SourceT*> getAll() const
	{
		// list of resulting sources
		std::vector<SourceT*> matches;
		
		// try to find SourceType in map
		std::map<SourceType, std::vector<Source*> >::const_iterator pos = type_map.find(source_type_of<SourceT>());
		
		// check if found
		if(pos == type_map.cend())
			return std::vector<SourceT*>();
		
		// add sources to list of results
		for(std::vector<Source*>::const_iterator it = pos->second.begin(); it != pos->second.end(); ++it)
			matches.push_back((SourceT*)(*it));
		
		// return sources of requested type
		return matches;
	}
	
	void printSource();
	
private:
	void assignParams(InputParams& params);
	
	/// add a source to the internal list
	void addInternal(Source* source, bool owned);
	
	/// read a Millenium galaxy data file
	void readGalaxyFile(std::string filename, Band band, double mag_limit);
	
	/// the current source index
	std::size_t index;
	
	/// list of sources and associated types
	std::vector<Source*> sources;
	
	/// map of sources and types
	std::map<SourceType, std::vector<Source*> > type_map;
	
	// TODO: handle creation better
	std::vector<Source*> created;
};

/**** inline functions ****/

inline double MultiSource::SurfaceBrightness(double *y) { return sources[index]->SurfaceBrightness(y); }

inline double MultiSource::getTotalFlux() { return sources[index]->getTotalFlux(); }

inline double MultiSource::getRadius() { return sources[index]->getRadius(); }

inline double MultiSource::getZ(){ return sources[index]->getZ(); }

inline void MultiSource::setZ(double z){ sources[index]->setZ(z); }

inline double* MultiSource::getX(){return sources[index]->getX(); }

inline void MultiSource::setX(double x[2]){ sources[index]->setX(x); }

inline void MultiSource::setX(double x1, double x2){ sources[index]->setX(x1, x2); }

inline std::size_t MultiSource::size() const { return sources.size(); }

inline Source* MultiSource::getCurrent() const { return sources[index]; }

inline std::size_t MultiSource::getIndex() const { return index; }

#endif
