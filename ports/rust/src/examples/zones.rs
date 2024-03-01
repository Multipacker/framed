use framed;
use framed::framed_zone;

#[framed_zone(name = "fib recursive")]
fn fib(n: i64) -> i64 {
    if n < 2 {
        n
    } else {
        fib(n - 2) + fib(n - 1)
    }
}

#[framed_zone]
fn fib_iter(n: i64) -> i64 {
    if n < 2 {
        n
    } else {
        let mut prev_prev = 0;
        let mut prev = 1;
        for _ in 2..=n {
            let new = prev_prev + prev;
            prev_prev = prev;
            prev = new;
        }
        prev
    }
}

fn main() {
    framed::init(true);
    for n in 0..30 {
        println!("fib({n}) = {}\nfib_iter({n}) = {}", fib(n), fib_iter(n));
    }
    framed::flush();
}
