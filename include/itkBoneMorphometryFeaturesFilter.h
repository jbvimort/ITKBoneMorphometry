/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef itkBoneMorphometryFeaturesFilter_h
#define itkBoneMorphometryFeaturesFilter_h

#include "itkImageToImageFilter.h"
#include "itkNumericTraits.h"
#include "itkSimpleDataObjectDecorator.h"
#include "itksys/hash_map.hxx"
#include "itkHistogram.h"
#include "itkConstNeighborhoodIterator.h"
#include "itkSimpleDataObjectDecorator.h"
#include "itkConstantBoundaryCondition.h"

#include <vector>

namespace itk
{
/** \class BoneMorphometryFeaturesFilter
 * \brief Compute the percent bone volume [BVTV], trabecular thickness [TbTh], trabecular separation [TbSp] trabecular number [TbN] and Bone Surface to Bone Volume ration [BSBV]
 *
 * BoneMorphometryFeaturesFilter computes bone morphometry features such as the percent bone volume [BVTV], the trabecular thickness [TbTh],
 * the trabecular separation [TbSp], the trabecular number [TbN], or the Bone Surface to Bone Volume ration [BSBV].
 * To do so, the filter needs a 3D input scan and a threshold. All voxels with an intensity higher than the threshold will
 * be considered as part of the bone.
 * A mask can also be specified in order to have more precise results (the morphometry will be computed only for the mask's voxels with a value of 1)
 *
 * BoneMorphometryFeaturesFilter behaves as a filter with an input and output. Thus it can be inserted
 * in a pipeline with other filters and the metrics will only be
 * recomputed if a downstream filter changes.
 *
 * The filter passes its input through unmodified.  The filter is
 * threaded. It computes metrics in each thread then combines them in
 * its AfterThreadedGenerate method.
 *
 * \author: Jean-Baptiste Vimort
 * \ingroup BoneMorphometry
 *
 */
template< typename TInputImage,typename TMaskImage = Image< unsigned char, TInputImage::ImageDimension> >
class ITK_TEMPLATE_EXPORT BoneMorphometryFeaturesFilter:
public ImageToImageFilter< TInputImage, TInputImage >
{
public:
  /** Standard Self typedef */
  typedef BoneMorphometryFeaturesFilter                   Self;
  typedef ImageToImageFilter< TInputImage, TInputImage >  Superclass;
  typedef SmartPointer< Self >                            Pointer;
  typedef SmartPointer< const Self >                      ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(BoneMorphometryFeaturesFilter, ImageToImageFilter);

  /** Image related typedefs. */
  typedef typename TInputImage::Pointer    InputImagePointer;
  typedef typename TInputImage::RegionType RegionType;
  typedef typename TInputImage::SizeType   SizeType;
  typedef typename TInputImage::IndexType  IndexType;
  typedef typename TInputImage::PixelType  PixelType;

  /** Mask related typedefs. */
  typedef typename TMaskImage::Pointer    MaskImagePointer;

  /** NeighborhoodIterator typedef */
  typedef ConstantBoundaryCondition< TInputImage >                           BoundaryConditionType;
  typedef ConstNeighborhoodIterator< TInputImage, BoundaryConditionType >    NeighborhoodIteratorType;
  typedef typename NeighborhoodIteratorType::RadiusType                      NeighborhoodRadiusType;
  typedef typename NeighborhoodIteratorType::OffsetType                      NeighborhoodOffsetType;

  /** Type to use for computations. */
  typedef typename NumericTraits< PixelType >::RealType RealType;

  /** Methods to set/get the mask image */
  itkSetInputMacro(MaskImage, TMaskImage);
  itkGetInputMacro(MaskImage, TMaskImage);

  /** Methods to set/get the mask image */
  itkSetMacro(Threshold, RealType);
  itkGetMacro(Threshold, RealType);

  /** Methods to get the mask different outputs */

  typedef SimpleDataObjectDecorator< RealType > RealTypeDecoratedType;

  RealType GetBVTV() { return m_Pp; }
  RealTypeDecoratedType * GetBVTVOutput()
  {
    typename RealTypeDecoratedType::Pointer decoratedBVTV = RealTypeDecoratedType::New();
    decoratedBVTV->Set( this->GetBVTV() );
    return decoratedBVTV.GetPointer();
  }

  RealType GetTbN() { return m_Pl; }
  RealTypeDecoratedType * GetTbNOutput()
  {
    typename RealTypeDecoratedType::Pointer decoratedTbN = RealTypeDecoratedType::New();
    decoratedTbN->Set( this->GetTbN() );
    return decoratedTbN.GetPointer();
  }

  RealType GetTbTh() { return m_Pp/m_Pl; }
  RealTypeDecoratedType * GetTbThOutput()
  {
    typename RealTypeDecoratedType::Pointer decoratedTbTh = RealTypeDecoratedType::New();
    decoratedTbTh->Set( this->GetTbTh() );
    return decoratedTbTh.GetPointer();
  }

  RealType GetTbSp() { return (1.0 - m_Pp) / m_Pl; }
  RealTypeDecoratedType * GetTbSpOutput()
  {
    typename RealTypeDecoratedType::Pointer decoratedTbSp = RealTypeDecoratedType::New();
    decoratedTbSp->Set( this->GetTbSp() );
    return decoratedTbSp.GetPointer();
  }

  RealType GetBSBV() { return 2.0 * (m_Pl / m_Pp); }
  RealTypeDecoratedType * GetBSBVOutput()
  {
    typename RealTypeDecoratedType::Pointer decoratedBSBV = RealTypeDecoratedType::New();
    decoratedBSBV->Set( this->GetBSBV() );
    return decoratedBSBV.GetPointer();
  }

#ifdef ITK_USE_CONCEPT_CHECKING
  // Begin concept checking
  itkConceptMacro( InputPixelDimensionCheck,
                   ( Concept::SameDimension<TInputImage::ImageDimension, 3u>) );
  // End concept checking
#endif


protected:
  BoneMorphometryFeaturesFilter();
  virtual ~BoneMorphometryFeaturesFilter() {}


  /** Pass the input through unmodified. Do this by Grafting in the
  *  AllocateOutputs method.
  */
  void AllocateOutputs() ITK_OVERRIDE;

  /** Initialize some accumulators before the threads run. */
  void BeforeThreadedGenerateData() ITK_OVERRIDE;

  /** Do final mean and variance computation from data accumulated in threads.
    */
  void AfterThreadedGenerateData() ITK_OVERRIDE;

  /** Multi-thread version GenerateData. */
  void  ThreadedGenerateData(const RegionType &
                             outputRegionForThread,
                             ThreadIdType threadId) ITK_OVERRIDE;

  void PrintSelf(std::ostream & os, Indent indent) const ITK_OVERRIDE;

private:

  //Inputs
  RealType m_Threshold;

  // Internal computation
  RealType m_Pp;
  RealType m_Pl;
  RealType m_PlX;
  RealType m_PlY;
  RealType m_PlZ;

  Array<SizeValueType> m_NumVoxelsInsideMask;
  Array<SizeValueType> m_NumBoneVoxels;
  Array<SizeValueType> m_NumX;
  Array<SizeValueType> m_NumY;
  Array<SizeValueType> m_NumZ;
  Array<SizeValueType> m_NumXO;
  Array<SizeValueType> m_NumYO;
  Array<SizeValueType> m_NumZO;

}; // end of class
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkBoneMorphometryFeaturesFilter.hxx"
#endif

#endif // itkBoneMorphometryFeaturesFilter_h
