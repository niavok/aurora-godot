extends Node

var gas_constant = 8.31446261815325 # Gas contant for mol J.mol-1.K-1
var gas_adiabatic_index = 1.3;
var gas_elastic_coef = gas_adiabatic_index / (gas_adiabatic_index - 1);
var gravity = 9.81 # m.s-2

enum Gas {
	#Hydrogen, #H2
	Nitrogen, #N2
	#Oxygen, #O2
	#Water,  #H2O
	#Methane, #CH4
	#CarbonDioxide, #CO2
	#SulfurDioxide, #SO2
	#Ammoniac, #NH4

	#U235F6, #UF6 with U235
	#U238F6, #UF6 with U238

	Count
}

var gas_specific_heat = [] # J.K-1.kg-1
var gas_mass_by_mole = [] # kg.mol-1
var gas_specific_heat_by_mole = [] # J.K-1.mol-1

func _ready():
	var gas
	gas_specific_heat.resize(Gas.Count)
	gas_mass_by_mole.resize(Gas.Count)
	gas_specific_heat_by_mole.resize(Gas.Count)

	# N2
	gas = Gas.Nitrogen
	gas_specific_heat[gas] = 1040.0
	gas_mass_by_mole[gas] = 0.0280134

	for i in range(Gas.Count):
		gas_specific_heat_by_mole[i] = gas_specific_heat[i] * gas_mass_by_mole[i]


func estimate_internal_energy(gas : int, N : float, temperature: float, volume : float) -> float:
	var pressure = compute_gas_pressure(N, volume, temperature)
	var elastic_energy = pressure * volume * gas_elastic_coef
	var thermal_energy = gas_specific_heat[gas] * gas_mass_by_mole[gas] * N * temperature
	return thermal_energy + elastic_energy


func estimate_gas_N(volume : float, pressure : float, temperature: float) -> float:
	# PV = NRT
	# N = PV / RT
	var N = pressure * volume / (gas_constant * temperature)
	return N;

func compute_gas_pressure( N : float, volume : float, temperature: float) -> float:
	# PV = NRT
	# P = NRT / V
	return N * gas_constant * temperature / volume
