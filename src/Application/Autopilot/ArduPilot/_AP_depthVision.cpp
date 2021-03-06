#include "../ArduPilot/_AP_depthVision.h"

#ifdef USE_OPENCV

namespace kai
{

_AP_depthVision::_AP_depthVision()
{
	m_pAP = NULL;
	m_pDV = NULL;
	m_nROI = 0;
}

_AP_depthVision::~_AP_depthVision()
{
}

bool _AP_depthVision::init(void* pKiss)
{
	IF_F(!this->_AutopilotBase::init(pKiss));
	Kiss* pK = (Kiss*) pKiss;

	//link
	string iName;

	iName = "";
	F_INFO(pK->v("APcopter_base", &iName));
	m_pAP = (_AP_base*) (pK->parent()->getChildInst(iName));

	iName = "";
	F_INFO(pK->v("_DepthVisionBase", &iName));
	m_pDV = (_DepthVisionBase*) (pK->root()->getChildInst(iName));
	IF_Fl(!m_pDV, iName + " not found");

	Kiss** pItrDS = pK->getChildItr();
	m_nROI = 0;

	while (pItrDS[m_nROI])
	{
		Kiss* pKs = pItrDS[m_nROI];
		IF_F(m_nROI >= N_DEPTH_ROI);

		DEPTH_ROI* pR = &m_pROI[m_nROI];
		pR->init();
		F_ERROR_F(pKs->v("orientation", (int*)&pR->m_orientation));
		F_ERROR_F(pKs->v("l", &pR->m_roi.x));
		F_ERROR_F(pKs->v("t", &pR->m_roi.y));
		F_ERROR_F(pKs->v("r", &pR->m_roi.z));
		F_ERROR_F(pKs->v("b", &pR->m_roi.w));

		m_nROI++;
	}

	return true;
}

void _AP_depthVision::update(void)
{
	this->_AutopilotBase::update();

	NULL_(m_pAP);
	NULL_(m_pAP->m_pMav);
	_Mavlink* pMavlink = m_pAP->m_pMav;
	NULL_(m_pDV);

	vFloat2 range = m_pDV->m_vRange;
	mavlink_distance_sensor_t D;

	for(int i=0; i<m_nROI; i++)
	{
		DEPTH_ROI* pR = &m_pROI[i];

		float d = m_pDV->d(&pR->m_roi);
		if(d <= range.x)d = range.y;
		if(d > range.y)d = range.y;
		pR->m_minD = d;

		D.type = 0;
		D.max_distance = (uint16_t)(range.y*100);	//unit: centimeters
		D.min_distance = (uint16_t)(range.x*100);
		D.current_distance = (uint16_t)(pR->m_minD * 100);
		D.orientation = pR->m_orientation;
		D.covariance = 255;

		pMavlink->distanceSensor(D);
		LOG_I("orient: " + i2str(pR->m_orientation) + " minD: " + f2str(pR->m_minD));
	}
}

void _AP_depthVision::draw(void)
{
	this->_AutopilotBase::draw();
	addMsg("nROI=" + i2str(m_nROI));

	IF_(!checkWindow());
	NULL_(m_pDV);

	Mat* pMat = ((Window*) this->m_pWindow)->getFrame()->m();

	for(int i=0; i<m_nROI; i++)
	{
		DEPTH_ROI* pR = &m_pROI[i];
		vFloat4 roi = pR->m_roi;
		float d = m_pDV->d(&roi);

		Rect r;
		r.x = roi.x * pMat->cols;
		r.y = roi.y * pMat->rows;
		r.width = roi.z * pMat->cols - r.x;
		r.height = roi.w * pMat->rows - r.y;
		rectangle(*pMat, r, Scalar(0, 255, 0), 1);

		putText(*pMat, f2str(d),
				Point(r.x + 15, r.y + 25),
				FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 255), 1);
	}
}

}
#endif
