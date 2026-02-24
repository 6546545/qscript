use crate::ast::Program;
use thiserror::Error;

#[derive(Debug, Error)]
pub enum TypeError {
    #[error("type checking is not yet implemented")]
    NotImplemented,
}

pub fn typecheck(program: &Program) -> Result<(), TypeError> {
    // Placeholder type checker: accept all programs for now.
    let _ = program;
    Ok(())
}

