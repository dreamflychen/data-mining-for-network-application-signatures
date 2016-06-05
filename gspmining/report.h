#ifndef REPORT_H
#define REPORT_H
#include "stdcommonheader.h"
#include "constdefine.h"
class CGspMining;
struct RelatedRuleCollection;
class CReport
{
public:
static void PrintCGspMiningInit(CGspMining *pGspMining);
static void PrintAuditInfoInCGspMining(CGspMining *pGspMining);
static void PrintAllLevelFragment(CGspMining *pGspMining);
static void PrintRelatedRule(CGspMining *pGspMining,RelatedRuleCollection *_prr);
static void PrintAllHalfWindowSizeSupport(CGspMining *pGspMining);
static void PrintAllShingleSizeSupport(CGspMining *pGspMining);
};

#endif