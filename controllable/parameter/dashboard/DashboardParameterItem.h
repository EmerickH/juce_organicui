#pragma once


class DashboardParameterItem :
	public DashboardControllableItem
{
public:
	DashboardParameterItem(Parameter* parameter = nullptr);
	~DashboardParameterItem();

	Parameter* parameter;

	virtual DashboardItemUI* createUI() override;

	virtual void setInspectableInternal(Inspectable* i) override;

	static DashboardParameterItem* create(var) { return new DashboardParameterItem(); }
	virtual String getTypeString() const override { return "DashboardParameterItem"; }

};