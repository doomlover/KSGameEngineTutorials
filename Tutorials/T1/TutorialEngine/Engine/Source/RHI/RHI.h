#pragma once

namespace ks
{
	class IRHI
	{
	public:
		virtual ~IRHI() {}
		static IRHI* Create();
		virtual void Init() = 0;
		virtual void Shutdown() = 0;
	};

	extern IRHI* GRHI;
}