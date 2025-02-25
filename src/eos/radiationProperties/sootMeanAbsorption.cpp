#include "sootMeanAbsorption.hpp"
#include "eos/tChemSoot.hpp"

ablate::eos::radiationProperties::SootMeanAbsorption::SootMeanAbsorption(std::shared_ptr<eos::EOS> eosIn) : eos(std::move(eosIn)) {}

PetscErrorCode ablate::eos::radiationProperties::SootMeanAbsorption::SootFunction(const PetscReal *conserved, PetscReal *kappa, void *ctx) {
    PetscFunctionBeginUser;
    /** This model depends on mass fraction, temperature, and density in order to predict the absorption properties of the medium. */
    auto functionContext = (FunctionContext *)ctx;
    double temperature, density;  //!< Variables to hold information gathered from the fields
                                  //!< Standard PETSc error code returned by PETSc functions

    PetscCall(functionContext->temperatureFunction.function(conserved, &temperature, functionContext->temperatureFunction.context.get()));   //!< Get the temperature value at this location
    PetscCall(functionContext->densityFunction.function(conserved, temperature, &density, functionContext->densityFunction.context.get()));  //!< Get the density value at this location

    PetscReal YinC = (functionContext->densityYiCSolidCOffset == -1) ? 0 : conserved[functionContext->densityYiCSolidCOffset] / density;  //!< Get the mass fraction of carbon here

    PetscReal fv = density * YinC / rhoC;

    *kappa = (3.72 * fv * C_0 * temperature) / C_2;

    PetscFunctionReturn(0);
}

PetscErrorCode ablate::eos::radiationProperties::SootMeanAbsorption::SootTemperatureFunction(const PetscReal *conserved, PetscReal temperature, PetscReal *kappa, void *ctx) {
    PetscFunctionBeginUser;
    /** This model depends on mass fraction, temperature, and density in order to predict the absorption properties of the medium. */
    auto functionContext = (FunctionContext *)ctx;
    double density;  //!< Variables to hold information gathered from the fields
                     //!< Standard PETSc error code returned by PETSc functions

    PetscCall(functionContext->densityFunction.function(conserved, temperature, &density, functionContext->densityFunction.context.get()));  //!< Get the density value at this location
    PetscReal YinC = (functionContext->densityYiCSolidCOffset == -1) ? 0 : conserved[functionContext->densityYiCSolidCOffset] / density;     //!< Get the mass fraction of carbon here

    PetscReal fv = density * YinC / rhoC;

    *kappa = (3.72 * fv * C_0 * temperature) / C_2;

    PetscFunctionReturn(0);
}

ablate::eos::ThermodynamicFunction ablate::eos::radiationProperties::SootMeanAbsorption::GetRadiationPropertiesFunction(RadiationProperty property, const std::vector<domain::Field> &fields) const {
    const auto densityYiField = std::find_if(fields.begin(), fields.end(), [](const auto &field) { return field.name == ablate::finiteVolume::CompressibleFlowFields::DENSITY_YI_FIELD; });

    /** Check if the species exist in this run.
     * If all don't exist, throw an error.
     * If some don't exist, then the values should be set to zero for their mass fractions in all cases.
     * */
    if (densityYiField == fields.end()) {
        throw std::invalid_argument("Soot absorption model requires the ablate::finiteVolume::CompressibleFlowFields::DENSITY_YI_FIELD.");
    }

    /** Get the offsets that locate the position of the solid carbon field. */
    PetscInt cOffset = (PetscInt)densityYiField->ComponentOffset(TChemSoot::CSolidName);

    if (cOffset == -1) {
        throw std::invalid_argument("Soot absorption model requires solid carbon.\n The Constant class allows the absorptivity of the medium to be set manually.");
    }

    switch (property) {
        case RadiationProperty::Absorptivity:
            return ThermodynamicFunction{.function = SootFunction,
                                         .context = std::make_shared<FunctionContext>(FunctionContext{
                                             .densityYiCSolidCOffset = cOffset,
                                             .temperatureFunction = eos->GetThermodynamicFunction(ThermodynamicProperty::Temperature, fields),
                                             .densityFunction = eos->GetThermodynamicTemperatureFunction(ThermodynamicProperty::Density, fields)})};  //!< Create a struct to hold the offsets
        default:
            throw std::invalid_argument("Unknown radiationProperties property in ablate::eos::radiationProperties::SootAbsorptionModel");
    }
}

ablate::eos::ThermodynamicTemperatureFunction ablate::eos::radiationProperties::SootMeanAbsorption::GetRadiationPropertiesTemperatureFunction(RadiationProperty property,
                                                                                                                                              const std::vector<domain::Field> &fields) const {
    const auto densityYiField = std::find_if(fields.begin(), fields.end(), [](const auto &field) { return field.name == ablate::finiteVolume::CompressibleFlowFields::DENSITY_YI_FIELD; });

    /** Check if the species exist in this run.
     * If all don't exist, throw an error.
     * If some don't exist, then the values should be set to zero for their mass fractions in all cases.
     * */
    if (densityYiField == fields.end()) {
        throw std::invalid_argument("Soot absorption model requires the density Yi field.");
    }

    /** Get the offsets that locate the position of the solid carbon field. */
    PetscInt cOffset = (PetscInt)densityYiField->ComponentOffset(TChemSoot::CSolidName);

    switch (property) {
        case RadiationProperty::Absorptivity:
            return ThermodynamicTemperatureFunction{
                .function = SootTemperatureFunction,
                .context = std::make_shared<FunctionContext>(
                    FunctionContext{.densityYiCSolidCOffset = cOffset,
                                    .temperatureFunction = eos->GetThermodynamicFunction(ThermodynamicProperty::Temperature, fields),
                                    .densityFunction = eos->GetThermodynamicTemperatureFunction(ThermodynamicProperty::Density, fields)})};  //!< Create a struct to hold the offsets
        default:
            throw std::invalid_argument("Unknown radiationProperties property in ablate::eos::radiationProperties::SootAbsorptionModel");
    }
}

#include "registrar.hpp"
REGISTER(ablate::eos::radiationProperties::RadiationModel, ablate::eos::radiationProperties::SootMeanAbsorption, "SootMeanAbsorption",
         ARG(ablate::eos::EOS, "eos", "The EOS used to compute field properties"));