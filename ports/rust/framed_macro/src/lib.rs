use proc_macro::{Span, TokenStream};
use std::ffi::CString;
use std::ops::Deref;
use syn::{parse, parse_quote, spanned::Spanned, ItemFn, LitStr, Visibility, Ident, Block, FnArg, Pat};
use quote::{ToTokens, format_ident};


#[proc_macro_attribute]
/// Automatically set up a zone for a function.
///
/// Without a `name = "custom name"` argument, the zone name will be derived from the
/// function name, otherwise the supplied name will be used.
///
/// Essentially equivalent to
///
/// ```
/// #[framed_zone(name="Foo")]
/// fn foo(x: i32) { /* ... */ }
/// ```
/// being turned into
///
/// ```
/// fn foo(x: i32) {
///     fn wrapped(x: i32) { /* ... */ }
///     framed::zone_begin(&CString::new("Foo").unwrap());
///     let result = wrapped(x);
///     framed::zone_end();
///     return result;
/// }
/// ```
///
/// # Limitations
///
/// The attribute cannot be applied to a function with
/// * variadic arguments or
/// * a `self` parameter (any variant).
///
/// # Errors
/// 
/// The attribute will raise a compilation error if the name (that of the function or the supplied) contains 
/// any non ascii characters or null bytes.
///
pub fn framed_zone(attr: TokenStream, item: TokenStream) -> TokenStream {
    let name = match get_zone_name_from_attributes(attr) {
        Ok(name) => name,
        Err(err) => return err,
    };

    let function_tree = match parse::<ItemFn>(item) {
        Ok(tree) => tree,
        Err(err) => {
            let span = err.span().unwrap();
            return error_message("#[framed_zone] attribute only applies to functions", span);
        }
    };
    let (zone_name, zone_span) = match name {
        Some(name) => (name.value(), name.span()),
        None => {
            let function_ident = &function_tree.sig.ident;
            (function_ident.to_string(), function_ident.span())
        }
    };

    let zone = if !zone_name.is_ascii() {
        return error_message("zone name needs to be valid ascii", zone_span.unwrap());
    } else {
        match CString::new(zone_name) {
            Ok(cname) => cname,
            Err(_) => {
                return error_message("zone name cannot contain null bytes", zone_span.unwrap());
            }
        }
    };
    
    let ItemFn { attrs, vis, mut sig, block } = function_tree;

    if sig.variadic.is_some() {
        todo!("handle variadic arguments to functions");
    }
    
    let mut inner_fun = ItemFn { 
        attrs: Vec::new(), 
        vis: Visibility::Inherited, 
        sig: sig.clone(),
        block,
    };

    inner_fun.sig.ident = Ident::new("wrapped", inner_fun.sig.ident.span());
    let variables: Vec<_> = sig.inputs.pairs_mut().enumerate().map(|(i, mut pair)| {
        match pair.value_mut() {
            FnArg::Receiver(_) => todo!("handle methods"),
            FnArg::Typed(typed) => {
                let varname = format_ident!("var{}", i); 
                let new_ident: Pat = parse_quote! { #varname };
                typed.pat = Box::new(new_ident);
                varname
            }
        }
    }).collect();
    

    let cstr = zone.into_bytes_with_nul();
    let cstr_slice = cstr.as_slice();
    let inner_block: Block = parse_quote! {
        {
            // Wrapper
            #inner_fun;
            let name = [ #(#cstr_slice),* ];
            let cname = unsafe { 
                // SAFE: the bytes are created in a safe way
                ::core::ffi::CStr::from_bytes_with_nul_unchecked(&name[..])
            };
            ::framed::zone_begin(cname);
            let r = wrapped( #(#variables),* );
            ::framed::zone_end();
            return r; 
        }
    };

    let new_function = ItemFn {
        attrs,
        vis,
        sig,
        block: Box::new(inner_block),
    };
    
    new_function.into_token_stream().into()
}

fn get_zone_name_from_attributes(attr: TokenStream) -> Result<Option<LitStr>, TokenStream> {
    if attr.is_empty() {
        Ok(None)
    } else {
        use syn::{Expr, ExprAssign, ExprLit, ExprPath, Lit};
        match parse::<ExprAssign>(attr) {
            Ok(tree) => {
                // Verify that the identifier is 'name'
                match &*tree.left {
                    Expr::Path(ExprPath { path, .. }) => {
                        match path.get_ident() {
                            Some(ident) if ident == "name" => Some(()), // name is the intended target
                            _ => None,
                        }
                    }
                    _ => None,
                }
                .ok_or(error_message(
                    "expected 'name = \"<custom zone name>\"'",
                    tree.span().unwrap(),
                ))?;

                // Get the string
                match &*tree.right {
                    Expr::Lit(ExprLit { lit, .. }) => match lit {
                        Lit::Str(string) => Ok(Some(string.clone())),
                        _ => Err(error_message(
                            "expected a string literal",
                            tree.right.deref().span().unwrap(),
                        )),
                    },
                    _ => Err(error_message(
                        "expected a string",
                        tree.right.deref().span().unwrap(),
                    )),
                }
            }
            Err(err) => Err(TokenStream::from(err.to_compile_error())),
        }
    }
}

/// Prints out `::core::compile_error!($message)` at the specified span
///
/// # Note
///
/// Adapted from https://docs.rs/syn/2.0.52/src/syn/error.rs.html#276-323
fn error_message(message: &str, span: Span) -> TokenStream {
    // TODO(RagePly): lookup any licensces that might be involved with the copy

    use proc_macro::{Delimiter, Group, Ident, Literal, Punct, Spacing, TokenTree};

    TokenStream::from_iter(vec![
        TokenTree::Punct({
            let mut punct = Punct::new(':', Spacing::Joint);
            punct.set_span(span);
            punct
        }),
        TokenTree::Punct({
            let mut punct = Punct::new(':', Spacing::Alone);
            punct.set_span(span);
            punct
        }),
        TokenTree::Ident(Ident::new("core", span)),
        TokenTree::Punct({
            let mut punct = Punct::new(':', Spacing::Joint);
            punct.set_span(span);
            punct
        }),
        TokenTree::Punct({
            let mut punct = Punct::new(':', Spacing::Alone);
            punct.set_span(span);
            punct
        }),
        TokenTree::Ident(Ident::new("compile_error", span)),
        TokenTree::Punct({
            let mut punct = Punct::new('!', Spacing::Alone);
            punct.set_span(span);
            punct
        }),
        TokenTree::Group({
            let mut group = Group::new(Delimiter::Brace, {
                TokenStream::from_iter(vec![TokenTree::Literal({
                    let mut string = Literal::string(message);
                    string.set_span(span);
                    string
                })])
            });
            group.set_span(span);
            group
        }),
    ])
}
