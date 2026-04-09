//
// Created by r6awe on 4/8/2026.
//

#include "Erosion.h"

void Erosion::Erode(ErodeProps props)
{
	for (int i = 0; i < props.iter; i++) {
		DoOneIteration(props);
	}
}
void Erosion::DoOneIteration(ErodeProps props) { }