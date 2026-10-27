#include "FpsHealthComponent.h"

void UFpsHealthComponent::ApplyDamage(float Amount)
{
	if (Amount <= 0.f || Health <= 0.f)
	{
		return;
	}
	Health = FMath::Max(0.f, Health - Amount);
	OnDamaged(Health);
}

bool UFpsHealthComponent::IsDepleted() const
{
	return Health <= 0.f;
}
