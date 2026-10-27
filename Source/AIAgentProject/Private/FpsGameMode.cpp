#include "FpsGameMode.h"

#include "FpsPawn.h"

AFpsGameMode::AFpsGameMode()
{
	DefaultPawnClass = AFpsPawn::StaticClass();
}
