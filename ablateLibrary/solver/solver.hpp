#ifndef ABLATELIBRARY_SOLVER_HPP
#define ABLATELIBRARY_SOLVER_HPP

#include <domain/region.hpp>
#include <domain/subDomain.hpp>
#include <parameters/parameters.hpp>
#include <string>
#include <vector>
#include "io/serializable.hpp"

namespace ablate::solver {

class TimeStepper;

class Solver : public io::Serializable {
   private:
    // pre and post step functions for the flow
    std::vector<std::function<void(TS ts, Solver&)>> preStepFunctions;
    std::vector<std::function<void(TS ts, Solver&, PetscReal)>> preStageFunctions;
    std::vector<std::function<void(TS ts, Solver&)>> postStepFunctions;
    std::vector<std::function<void(TS ts, Solver&)>> postEvaluateFunctions;

    // The name of this solver
    const std::string solverId;

    // The region of this solver.
    const std::shared_ptr<domain::Region> region;

   protected:
    // an optional petscOptions that is used for this solver
    PetscOptions petscOptions;

    // use the subDomain to setup the problem
    std::shared_ptr<ablate::domain::SubDomain> subDomain;

    // The constructor to be call by any Solve implementation
    explicit Solver(std::string solverId, std::shared_ptr<domain::Region> = {}, std::shared_ptr<parameters::Parameters> options = nullptr);

    // Replacement calls for PETSC versions allowing multiple DS
    static PetscErrorCode DMPlexInsertBoundaryValues_Plex(DM dm, PetscDS ds, PetscBool insertEssential, Vec locX, PetscReal time, Vec faceGeomFVM, Vec cellGeomFVM, Vec gradFVM);
    static PetscErrorCode DMPlexInsertTimeDerivativeBoundaryValues_Plex(DM dm, PetscDS ds, PetscBool insertEssential, Vec locX, PetscReal time, Vec faceGeomFVM, Vec cellGeomFVM, Vec gradFVM);

   public:
    virtual ~Solver();

    /** Register all needed fields with the subDomain **/
    virtual void Register(std::shared_ptr<ablate::domain::SubDomain> subDomain);

    /** Setup and size the subDomain with the subDomain **/
    virtual void Setup() = 0;

    /** Finalize the Setup of the subDomain before running **/
    virtual void Initialize() = 0;

    /**
     * Get the sub domain used in this solver
     * @return
     */
    inline ablate::domain::SubDomain& GetSubDomain() noexcept { return *subDomain; }

    /**
     * Get the region used to define this solver
     * @return
     */
    inline std::shared_ptr<domain::Region> GetRegion() const noexcept { return region; }

    inline const std::string& GetId() const override { return solverId; }

    // Support for timestepping calls
    void PreStage(TS ts, PetscReal stagetime);
    void PreStep(TS ts);
    void PostStep(TS ts);
    void PostEvaluate(TS ts);

    /**
     * Adds function to be called before each flow step
     * @param preStep
     */
    inline void RegisterPreStep(std::function<void(TS ts, Solver&)> preStep) { this->preStepFunctions.push_back(preStep); }

    /**
     * Adds function to be called before each flow step
     * @param preStep
     */
    inline void RegisterPreStage(std::function<void(TS ts, Solver&, PetscReal)> preStage) { this->preStageFunctions.push_back(preStage); }

    /**
     * Adds function to be called after each flow step
     * @param preStep
     */
    inline void RegisterPostStep(std::function<void(TS ts, Solver&)> postStep) { this->postStepFunctions.push_back(postStep); }

    /**
     * Adds function after each evaluated.  This is where the solution can be modified if needed.
     * @param postStep
     */
    inline void RegisterPostEvaluate(std::function<void(TS ts, Solver&)> postEval) { this->postEvaluateFunctions.push_back(postEval); }

    void Save(PetscViewer viewer, PetscInt sequenceNumber, PetscReal time) const override;
    void Restore(PetscViewer viewer, PetscInt sequenceNumber, PetscReal time) override;

    /**
     * Get the cellIS and range over valid cells in this region
     * @param cellIS
     * @param pStart
     * @param pEnd
     * @param points
     */
    void GetCellRange(IS& cellIS, PetscInt& cStart, PetscInt& cEnd, const PetscInt*& cells);

    /**
     * Get the faceIS and range over valid faces in this region
     * @param cellIS
     * @param pStart
     * @param pEnd
     * @param points
     */
    void GetFaceRange(IS& faceIS, PetscInt& fStart, PetscInt& fEnd, const PetscInt*& faces);

    /**
     * Get the valid range over specified depth
     * @param cellIS
     * @param pStart
     * @param pEnd
     * @param points
     */
    void GetRange(PetscInt depth, IS& pointIS, PetscInt& pStart, PetscInt& pEnd, const PetscInt*& points);

    /**
     * Restores the is and range
     * @param cellIS
     * @param pStart
     * @param pEnd
     * @param points
     */
    void RestoreRange(IS& pointIS, PetscInt& pStart, PetscInt& pEnd, const PetscInt*& points);
};

}  // namespace ablate::solver
#endif  // ABLATELIBRARY_SOLVER_HPP
